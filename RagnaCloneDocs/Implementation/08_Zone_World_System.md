# 08 -- Zone / World System: UE5 C++ Implementation Guide

> Complete implementation reference for the Sabri_MMO zone/map/world system. Covers zone architecture, zone transitions, warp portals, Kafra NPCs, landscape creation, environment art, minimap, map flags, and server-side zone management. All code examples are production-ready and drawn from the existing codebase.

---

## Table of Contents

1. [Zone Architecture Overview](#1-zone-architecture-overview)
2. [Creating a New Zone (Step-by-Step)](#2-creating-a-new-zone-step-by-step)
3. [Zone Transition System](#3-zone-transition-system)
4. [Warp Portal System](#4-warp-portal-system)
5. [Kafra NPC System](#5-kafra-npc-system)
6. [Landscape / Terrain Creation](#6-landscape--terrain-creation)
7. [Environment Art Placement](#7-environment-art-placement)
8. [Minimap Data](#8-minimap-data)
9. [Map Properties / Flags](#9-map-properties--flags)
10. [Server-Side Zone Management](#10-server-side-zone-management)

---

## 1. Zone Architecture Overview

### 1.1 Core Principle: One Zone = One UE5 Level

Every zone in Sabri_MMO maps to exactly one UE5 level file. There is no level streaming or world partition -- each zone is a self-contained level loaded via `UGameplayStatics::OpenLevel()`. This matches the original Ragnarok Online model where each map is a discrete area.

**Naming convention:** All level files use the `L_` prefix and are saved under `Content/SabriMMO/Levels/`.

```
L_Prontera          -- Prontera town
L_PrtSouth          -- Prontera South Field (prt_fild08)
L_PrtNorth          -- Prontera North Field (prt_fild01)
L_PrtDungeon01      -- Prontera Culvert Floor 1
L_Geffen            -- Geffen town
L_GefDungeon01      -- Geffen Tower Floor 1
L_Payon             -- Payon town
L_PayDungeon01      -- Payon Cave Floor 1
L_Morroc            -- Morroc town
```

**Never** use `Map_` or other prefixes. **Never** create a level from scratch -- always duplicate an existing working level so the Level Blueprint, World Settings, and actor configuration are preserved.

### 1.2 Zone Registry on Server (`ro_zone_data.js`)

The server maintains a canonical registry of all zones in `server/src/ro_zone_data.js`. Each zone entry defines:

```javascript
const ZONE_REGISTRY = {
    prontera: {
        name: 'prontera',                          // Unique zone ID (used in DB, socket rooms, queries)
        displayName: 'Prontera',                    // Human-readable name shown in UI
        type: 'town',                               // 'town' | 'field' | 'dungeon'
        flags: {                                    // RO map flags (see Section 9)
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: true,
            indoor: false
        },
        defaultSpawn: { x: -240, y: -1700, z: 590 },  // Where players appear when entering
        levelName: 'L_Prontera',                        // UE5 level file name for OpenLevel()
        warps: [                                        // Warp portal definitions (see Section 4)
            {
                id: 'prt_south_exit',
                x: 30, y: -2650, z: 490,
                radius: 200,
                destZone: 'prontera_south',
                destX: 1330, destY: 0, destZ: 90
            }
        ],
        kafraNpcs: [                                    // Kafra NPC definitions (see Section 5)
            {
                id: 'kafra_prontera_1',
                name: 'Kafra Employee',
                x: 200, y: -1800, z: 300,
                destinations: [
                    { zone: 'prontera_south', displayName: 'Prontera South Field', cost: 100 }
                ]
            }
        ],
        enemySpawns: []                                 // Monster spawn configs (empty for towns)
    }
};
```

**Helper functions** exported from the module:

```javascript
function getZone(zoneName) {
    return ZONE_REGISTRY[zoneName] || null;
}

function getAllEnemySpawns() {
    const allSpawns = [];
    for (const [zoneName, zone] of Object.entries(ZONE_REGISTRY)) {
        for (const spawn of zone.enemySpawns) {
            allSpawns.push({ ...spawn, zone: zoneName });
        }
    }
    return allSpawns;
}

function getZoneNames() {
    return Object.keys(ZONE_REGISTRY);
}

module.exports = { ZONE_REGISTRY, getZone, getAllEnemySpawns, getZoneNames };
```

### 1.3 Zone-Scoped Socket.io Rooms

Every player socket joins a Socket.io room named `zone:<zoneName>`. Broadcasting is always zone-scoped -- players in Prontera never receive combat events from Prontera South Field.

```javascript
// Player joins a zone room on login or warp
socket.join('zone:' + playerZone);

// Broadcasting helpers
function broadcastToZone(zone, event, data) {
    io.to('zone:' + zone).emit(event, data);
}
function broadcastToZoneExcept(socket, zone, event, data) {
    socket.to('zone:' + zone).emit(event, data);
}
```

When a player warps to a new zone, the server:
1. Removes them from the old room: `socket.leave('zone:' + oldZone)`
2. Adds them to the new room: `socket.join('zone:' + newZone)`
3. Broadcasts `player:left` to the old zone
4. Broadcasts `player:moved` to the new zone (after `zone:ready`)

### 1.4 Player Tracking Per Zone

The server tracks each player's current zone in two places:

1. **In-memory** (`connectedPlayers` Map): `player.zone` field, updated on every warp
2. **Database** (`characters` table): `zone_name` column, persisted on warp + disconnect

```javascript
// Get a player's current zone (in-memory)
function getPlayerZone(characterId) {
    const player = connectedPlayers.get(characterId);
    return player ? (player.zone || 'prontera_south') : 'prontera_south';
}

// Check if any player is in a zone (for AI tick optimization)
function isZoneActive(zone) {
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone === zone) return true;
    }
    return false;
}

// Get all zones with at least one player
function getActiveZones() {
    const zones = new Set();
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone) zones.add(player.zone);
    }
    return zones;
}
```

### 1.5 Zone Transition Flow (High-Level)

```
Player enters warp portal overlap
    |
    v
Client: AWarpPortal::OnOverlapBegin()
    |-- Calls ZoneTransitionSubsystem::RequestWarp(WarpId)
    |-- Emits socket event: zone:warp { warpId }
    v
Server: zone:warp handler
    |-- Validates: player exists, zone exists, warp exists, proximity check
    |-- Cancels active combat/casting
    |-- Leaves old zone room, joins new zone room
    |-- Updates player.zone + saves to DB
    |-- Lazy-spawns enemies in new zone if first visit
    |-- Emits to client: zone:change { zone, displayName, levelName, x, y, z, flags }
    v
Client: ZoneTransitionSubsystem::HandleZoneChange()
    |-- Stores PendingZoneName, PendingLevelName, PendingSpawnLocation in GameInstance
    |-- Sets bIsZoneTransitioning = true
    |-- Shows loading overlay (Z=50, fullscreen, opaque)
    |-- Calls UGameplayStatics::OpenLevel(levelName)
    v
UE5 Level Transition
    |-- Old world is destroyed (all subsystems Deinitialize)
    |-- New level loads
    |-- New world begins play (all subsystems Initialize + OnWorldBeginPlay)
    v
New Level: Level Blueprint
    |-- BeginPlay -> Delay 0.2s -> GameInstance -> SpawnActor BP_MMOCharacter -> Possess
    |-- BP_MMOCharacter::BeginPlay checks bIsZoneTransitioning, teleports to PendingSpawnLocation
    v
New Level: ZoneTransitionSubsystem::OnWorldBeginPlay()
    |-- Detects bIsZoneTransitioning = true
    |-- Shows loading overlay
    |-- Starts polling (0.3s) via CheckTransitionComplete()
    v
ZoneTransitionSubsystem::CheckTransitionComplete() (polled)
    |-- Waits for socket events to wrap (socket reconnected)
    |-- Waits for player pawn to exist
    |-- Teleports pawn to PendingSpawnLocation (bPawnTeleported flag)
    |-- Updates CurrentZoneName
    |-- Emits zone:ready to server
    |-- Hides loading overlay
    v
Server: zone:ready handler
    |-- Broadcasts player:moved to zone (announces arrival)
    |-- Sends combat:health_update to zone
    |-- Sends all zone enemies to this client (enemy:spawn)
    |-- Sends all other players in zone to this client
    |-- Removes from zoneTransitioning set
    v
TRANSITION COMPLETE
```

### 1.6 State Persistence Across Level Transitions

`UGameInstance` survives level transitions. The following state is stored on `UMMOGameInstance`:

```cpp
// MMOGameInstance.h -- Zone System (survives level transitions)
UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
FString CurrentZoneName = TEXT("prontera_south");

UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
FString PendingZoneName;

UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
FString PendingLevelName;

UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
FVector PendingSpawnLocation = FVector::ZeroVector;

UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
bool bIsZoneTransitioning = false;
```

**Critical rule:** Never store cross-level state in PlayerController, GameMode, Pawn, or any WorldSubsystem. Only GameInstance persists across `OpenLevel()`.

---

## 2. Creating a New Zone (Step-by-Step)

### 2.1 Prerequisites

Before creating any new zone, ensure:
- Server compiled and running (`cd server && npm run dev`)
- Database migration applied: `database/migrations/add_zone_system.sql`
- C++ compiled: `WarpPortal`, `KafraNPC`, `ZoneTransitionSubsystem`, `KafraSubsystem`, `SKafraWidget`
- At least one working level exists to duplicate (e.g., `L_PrtSouth`)

### 2.2 Step 1: Add Zone to Server Registry

Edit `server/src/ro_zone_data.js` and add a new entry to `ZONE_REGISTRY`:

```javascript
// Example: Adding Geffen town
geffen: {
    name: 'geffen',
    displayName: 'Geffen',
    type: 'town',
    flags: {
        noteleport: false,
        noreturn: false,
        nosave: false,
        pvp: false,
        town: true,
        indoor: false
    },
    defaultSpawn: { x: 0, y: 0, z: 590 },
    levelName: 'L_Geffen',
    warps: [
        {
            id: 'gef_south_exit',
            x: 0, y: -3000, z: 490,
            radius: 200,
            destZone: 'geffen_south_field',
            destX: 0, destY: 1500, destZ: 90
        },
        {
            id: 'gef_north_exit',
            x: 0, y: 3000, z: 490,
            radius: 200,
            destZone: 'geffen_north_field',
            destX: 0, destY: -1500, destZ: 90
        }
    ],
    kafraNpcs: [
        {
            id: 'kafra_geffen_1',
            name: 'Kafra Employee',
            x: 300, y: -200, z: 300,
            destinations: [
                { zone: 'prontera', displayName: 'Prontera', cost: 600 },
                { zone: 'payon', displayName: 'Payon', cost: 1200 },
                { zone: 'alberta', displayName: 'Alberta', cost: 1200 },
                { zone: 'morroc', displayName: 'Morroc', cost: 1200 }
            ]
        }
    ],
    enemySpawns: []  // Towns have no enemies
},
```

**Key rules:**
- `name` must be unique and lowercase (used as DB value and socket room name)
- `levelName` must exactly match the UE5 level asset name
- Warp `id` must be unique within the zone
- Warp `destZone` must reference a valid zone `name` in the registry
- Kafra `id` must be unique across all zones
- Coordinate system: UE units (50 UE units ~ 1 RO cell)

### 2.3 Step 2: Add Connecting Warps

If the new zone connects to existing zones, add warp definitions in the existing zones too. Warps are always bidirectional by convention:

```javascript
// In the existing 'prontera' zone, add a warp TO geffen fields:
{
    id: 'prt_west_exit',
    x: -3500, y: 0, z: 490,
    radius: 200,
    destZone: 'geffen_east_field',
    destX: 1500, destY: 0, destZ: 90
}
```

### 2.4 Step 3: Duplicate an Existing Level

**ALWAYS duplicate an existing level. Never create from scratch.**

The Level Blueprint is critical and is NOT copied when creating a "new empty level." Duplicating preserves:
- Level Blueprint (character spawn, position save timer, cleanup)
- World Settings (GameMode override = GM_MMOGameMode, DefaultPawnClass = None)
- Required Blueprint actors (BP_SocketManager, BP_OtherPlayerManager, BP_EnemyManager)
- NavMesh Bounds Volume
- Lighting setup

**Via UE5 Editor:**
1. Content Browser -> navigate to `Content/SabriMMO/Levels/`
2. Right-click an existing level (e.g., `L_PrtSouth`) -> Duplicate
3. Rename the duplicate to `L_Geffen` (must match `levelName` in registry)
4. Open the new level
5. Delete zone-specific content from the template (enemy spawns, old warp portals, old terrain)
6. Build your new zone content

### 2.5 Step 4: Level Blueprint Structure

The Level Blueprint for every game level has three sections. When you duplicate a level, this is already set up. Verify it matches this structure:

**A. Spawn and Possess Character (Event BeginPlay):**

```
Event BeginPlay
    -> Delay 0.2s
    -> Cast To MMOGameInstance (from Get Game Instance)
    -> Get Selected Character -> Break Character Data
        -> Extract: CharacterId, X, Y, Z
    -> Branch: CharacterId > 0 (was a character selected?)
    -> Branch: Vector Length Squared(Make Vector(X,Y,Z)) != 0.0 (has saved location?)
        TRUE:  SpawnActor BP_MMOCharacter at (X, Y, Z) -> Possess (Get Player Controller 0)
        FALSE: SpawnActor BP_MMOCharacter at (0, 0, 900) -> Possess (Get Player Controller 0)
    -> Set Timer by Function Name ("SaveCharacterPosition", Time=5.0, Looping=true)
```

**B. SaveCharacterPosition (Custom Event, called every 5s):**

```
Cast To MMOGameInstance
    -> Get Selected Character -> Break Character Data -> CharacterId
Get Player Character (0)
    -> Get Actor Location -> X, Y, Z
-> Save Character Position (CharacterId, X, Y, Z)
```

**C. Cleanup (Event End Play):**

```
Event End Play
    -> Clear Timer by Function Name ("SaveCharacterPosition")
```

**NOTE:** The Level Blueprint cannot be created or modified via C++ or Claude Code -- it requires manual Blueprint editing in the UE5 Editor. This is the one part of zone creation that requires manual work. Duplicating a level copies this Blueprint automatically.

### 2.6 Step 5: World Settings

Verify in World Settings (Window -> World Settings):
- **GameMode Override** = `GM_MMOGameMode`
- Open `GM_MMOGameMode` Blueprint -> Class Defaults -> **Default Pawn Class = None**

**DefaultPawnClass must be None.** The Level Blueprint handles spawning `BP_MMOCharacter`. If the GameMode also has a DefaultPawnClass set, UE5 will spawn a duplicate pawn at the PlayerStart/world origin.

### 2.7 Step 6: Place Required Blueprint Actors

Every game level must contain these actors placed in the world:

| Blueprint Actor | Location | Purpose |
|----------------|----------|---------|
| `BP_SocketManager` | (0, 0, 0) | Socket.io connection hub |
| `BP_OtherPlayerManager` | (0, 0, 0) | Remote player spawning and interpolation |
| `BP_EnemyManager` | (0, 0, 0) | Enemy spawning and management |

These are placed via the UE5 Editor Content Browser (drag from `Content/SabriMMO/Blueprints/`).

### 2.8 Step 7: Place Warp Portal Actors

For each warp defined in `ro_zone_data.js` for this zone, place an `AWarpPortal` C++ actor:

```
// For the warp: { id: 'gef_south_exit', x: 0, y: -3000, z: 490, radius: 200, ... }

Place AWarpPortal actor at location (0, -3000, 490)
Set properties:
    WarpId = "gef_south_exit"     // Must match the warp 'id' in ro_zone_data.js
    TriggerRadius = 200           // Must match the warp 'radius' in ro_zone_data.js
```

**CRITICAL: Keep Warp Positions in Sync.** The server validates player proximity to warp portals. If you move an `AWarpPortal` actor in UE5, you MUST update the matching warp's `x, y, z` in `ro_zone_data.js`. Otherwise players get "Too far from warp portal" errors.

The warp `x, y, z` = the UE5 actor's location (where the portal trigger sphere is). The warp `destX, destY, destZ` = where the player spawns in the destination zone.

### 2.9 Step 8: Place Kafra NPC Actors

For each Kafra NPC defined in `ro_zone_data.js`:

```
// For: { id: 'kafra_geffen_1', name: 'Kafra Employee', x: 300, y: -200, z: 300, ... }

Place AKafraNPC actor at location (300, -200, 300)
Set properties:
    KafraId = "kafra_geffen_1"              // Must match the kafra 'id' in ro_zone_data.js
    NPCDisplayName = "Kafra Employee"       // Display name shown above NPC head
    InteractionRadius = 300.0               // How close player must be to interact (UE units)
```

### 2.10 Step 9: NavMesh

Place a `Nav Mesh Bounds Volume` covering the entire playable area:
- Location: center of the playable area, slightly above ground
- Scale: large enough to cover all walkable terrain (e.g., Scale (80, 80, 10))
- Build navigation: Build -> Build Paths

### 2.11 Step 10: Lighting

**Outdoor zones (fields, towns):**
- Directional Light (sunlight): pitch -45 degrees, yaw 0
- Sky Atmosphere + Sky Light for ambient lighting
- Optionally: Exponential Height Fog

**Indoor zones (dungeons):**
- NO directional light or sky
- Point Lights at corridor intersections (warm orange, intensity 500, radius 300)
- Spotlights near entrances and special areas

### 2.12 Step 11: Monster Spawn Points

Spawn points are defined entirely in the server registry (`ro_zone_data.js`). No UE5-side configuration needed -- the server sends `enemy:spawn` events when a player enters the zone, and `BP_EnemyManager` handles instantiation.

```javascript
enemySpawns: [
    { template: 'poring',   x: 300,  y: 300,  z: 300, wanderRadius: 400 },
    { template: 'lunatic',  x: 600,  y: 500,  z: 300, wanderRadius: 350 },
    { template: 'fabre',    x: -600, y: -400, z: 300, wanderRadius: 300 },
]
```

The `template` name must match a key in `server/src/ro_monster_templates.js`. The `wanderRadius` controls how far the monster roams from its spawn point in IDLE state.

### 2.13 Complete Checklist for a New Zone

```
[ ] 1. Zone added to ro_zone_data.js (name, displayName, type, flags, defaultSpawn, levelName, warps, kafraNpcs, enemySpawns)
[ ] 2. Connecting warps added to adjacent zones in ro_zone_data.js
[ ] 3. Level duplicated from existing working level (NOT created from scratch)
[ ] 4. Level renamed to match levelName in registry (L_ prefix)
[ ] 5. Level Blueprint verified (BeginPlay spawn + SaveCharacterPosition timer + EndPlay cleanup)
[ ] 6. World Settings -> GameMode Override = GM_MMOGameMode (DefaultPawnClass = None)
[ ] 7. BP_SocketManager placed in level
[ ] 8. BP_OtherPlayerManager placed in level
[ ] 9. BP_EnemyManager placed in level
[ ] 10. AWarpPortal actors placed (one per warp in registry, WarpId matching)
[ ] 11. AKafraNPC actors placed (one per kafra in registry, KafraId matching)
[ ] 12. Nav Mesh Bounds Volume covering playable area
[ ] 13. Lighting setup (outdoor or dungeon)
[ ] 14. Terrain/geometry placed
[ ] 15. Server restarted to pick up new zone data
[ ] 16. Tested: warp into zone, warp out, Kafra services, enemy spawns
```

---

## 3. Zone Transition System

### 3.1 `UZoneTransitionSubsystem` -- Overview

**File:** `client/SabriMMO/Source/SabriMMO/UI/ZoneTransitionSubsystem.h` / `.cpp`

`UZoneTransitionSubsystem` is a `UWorldSubsystem` that manages all zone changes. It wraps three Socket.io events and provides the `RequestWarp()` API used by `AWarpPortal` actors.

```cpp
// ZoneTransitionSubsystem.h
UCLASS()
class SABRIMMO_API UZoneTransitionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Zone data (read by other systems)
    FString CurrentZoneName;
    FString CurrentDisplayName;
    bool bNoTeleport = false;
    bool bNoReturn = false;
    bool bNoSave = false;
    bool bIsTown = false;

    // Public API
    void RequestWarp(const FString& WarpId);

    // Lifecycle
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    // Socket event wrapping
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // Event handlers
    void HandleZoneChange(const TSharedPtr<FJsonValue>& Data);
    void HandleZoneError(const TSharedPtr<FJsonValue>& Data);
    void HandlePlayerTeleport(const TSharedPtr<FJsonValue>& Data);

    // Transition management
    void CheckTransitionComplete();
    void TeleportPawnToSpawn();
    void ForceCompleteTransition();

    // Loading overlay
    void ShowLoadingOverlay(const FString& StatusText);
    void HideLoadingOverlay();

    // State
    bool bEventsWrapped = false;
    bool bPawnTeleported = false;
    int32 LocalCharacterId = 0;
    int32 TransitionCheckCount = 0;
    FTimerHandle BindCheckTimer;
    FTimerHandle TransitionCheckTimer;

    TSharedPtr<SWidget> LoadingWidget;
    TSharedPtr<SWidget> LoadingOverlay;
    bool bLoadingShown = false;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 3.2 Lifecycle

**ShouldCreateSubsystem:** Only creates in game worlds (not editor preview):

```cpp
bool UZoneTransitionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}
```

**OnWorldBeginPlay:** Seeds zone name from GameInstance, starts polling for socket component, and handles mid-transition resumption:

```cpp
void UZoneTransitionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Seed zone name from GameInstance
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
    {
        CurrentZoneName = GI->CurrentZoneName;
    }

    // Start polling for socket events (0.5s interval)
    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UZoneTransitionSubsystem::TryWrapSocketEvents),
        0.5f, true
    );

    // If mid-transition (level just loaded after warp), show loading and wait for socket
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
    {
        if (GI->bIsZoneTransitioning)
        {
            ShowLoadingOverlay(FString::Printf(TEXT("Entering %s..."), *GI->PendingZoneName));
            TransitionCheckCount = 0;
            bPawnTeleported = false;

            InWorld.GetTimerManager().SetTimer(
                TransitionCheckTimer,
                FTimerDelegate::CreateUObject(this, &UZoneTransitionSubsystem::CheckTransitionComplete),
                0.3f, true
            );
        }
    }
}
```

**Deinitialize:** Cleans up loading overlay and timers:

```cpp
void UZoneTransitionSubsystem::Deinitialize()
{
    HideLoadingOverlay();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
        World->GetTimerManager().ClearTimer(TransitionCheckTimer);
    }
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}
```

### 3.3 Socket Event Wrapping

The subsystem wraps three events by intercepting the SocketIO native client's event function map. The original BP callback is preserved and called first:

```cpp
void UZoneTransitionSubsystem::WrapSingleEvent(
    const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
    if (!NativeClient.IsValid()) return;

    // Capture existing BP callback
    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
    FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
    if (Existing)
    {
        OriginalCallback = Existing->Function;
    }

    // Replace with wrapper that calls both
    NativeClient->OnEvent(EventName,
        [OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
        {
            if (OriginalCallback) OriginalCallback(Event, Message);
            if (OurHandler) OurHandler(Message);
        },
        TEXT("/"),
        ESIOThreadOverrideOption::USE_GAME_THREAD
    );
}
```

Events wrapped:
- `zone:change` -- Server tells client to load a new level
- `zone:error` -- Server reports a zone operation error (invalid warp, too far, etc.)
- `player:teleport` -- Server tells client to move pawn (Fly Wing, Butterfly Wing)

### 3.4 HandleZoneChange -- The Core Transition Trigger

```cpp
void UZoneTransitionSubsystem::HandleZoneChange(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    // Parse zone change data
    FString Zone, DisplayName, LevelName;
    Obj->TryGetStringField(TEXT("zone"), Zone);
    Obj->TryGetStringField(TEXT("displayName"), DisplayName);
    Obj->TryGetStringField(TEXT("levelName"), LevelName);

    double X = 0, Y = 0, Z = 0;
    Obj->TryGetNumberField(TEXT("x"), X);
    Obj->TryGetNumberField(TEXT("y"), Y);
    Obj->TryGetNumberField(TEXT("z"), Z);

    UWorld* World = GetWorld();
    if (!World) return;

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
    if (!GI) return;

    // Store transition data in GameInstance (survives level transition)
    GI->PendingZoneName = Zone;
    GI->PendingLevelName = LevelName;
    GI->PendingSpawnLocation = FVector(X, Y, Z);
    GI->bIsZoneTransitioning = true;

    // Parse zone flags
    const TSharedPtr<FJsonObject>* FlagsPtr = nullptr;
    if (Obj->TryGetObjectField(TEXT("flags"), FlagsPtr) && FlagsPtr)
    {
        const TSharedPtr<FJsonObject>& Flags = *FlagsPtr;
        bool bVal = false;
        if (Flags->TryGetBoolField(TEXT("noteleport"), bVal)) bNoTeleport = bVal;
        if (Flags->TryGetBoolField(TEXT("noreturn"), bVal)) bNoReturn = bVal;
        if (Flags->TryGetBoolField(TEXT("nosave"), bVal)) bNoSave = bVal;
        if (Flags->TryGetBoolField(TEXT("town"), bVal)) bIsTown = bVal;
    }

    // Show loading overlay
    ShowLoadingOverlay(FString::Printf(TEXT("Entering %s..."), *DisplayName));

    // Open the new level after a brief delay (let overlay render one frame)
    World->GetTimerManager().SetTimerForNextTick([World, LevelName]()
    {
        if (World && !LevelName.IsEmpty())
        {
            UGameplayStatics::OpenLevel(World, *LevelName);
        }
    });
}
```

### 3.5 CheckTransitionComplete -- Polling for Readiness

After the new level loads, this function polls every 0.3s until both conditions are met:
1. Socket events are wrapped (connection re-established)
2. Player pawn exists (Level Blueprint spawned it)

```cpp
void UZoneTransitionSubsystem::CheckTransitionComplete()
{
    TransitionCheckCount++;

    // Wait until socket events are wrapped
    if (!bEventsWrapped) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
    if (!GI || !GI->bIsZoneTransitioning)
    {
        // Transition was cancelled externally
        World->GetTimerManager().ClearTimer(TransitionCheckTimer);
        HideLoadingOverlay();
        return;
    }

    // Wait for player pawn to exist
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn())
    {
        // Timeout after ~10 seconds (33 checks at 0.3s)
        if (TransitionCheckCount >= 33)
        {
            ForceCompleteTransition();
        }
        return;
    }

    // Teleport pawn to correct position (only once)
    if (!bPawnTeleported)
    {
        TeleportPawnToSpawn();
        bPawnTeleported = true;
    }

    // Update zone state
    GI->CurrentZoneName = GI->PendingZoneName;
    CurrentZoneName = GI->CurrentZoneName;
    CurrentDisplayName = GI->PendingZoneName;
    GI->bIsZoneTransitioning = false;

    // Emit zone:ready to server
    if (CachedSIOComponent.IsValid())
    {
        TSharedPtr<FJsonObject> ReadyPayload = MakeShareable(new FJsonObject());
        ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
        CachedSIOComponent->EmitNative(TEXT("zone:ready"), ReadyPayload);
    }

    // Clear timer and hide loading
    World->GetTimerManager().ClearTimer(TransitionCheckTimer);
    World->GetTimerManager().SetTimerForNextTick([this]()
    {
        HideLoadingOverlay();
    });
}
```

### 3.6 Loading Overlay

The loading overlay is a fullscreen Slate widget at Z-order 50 (above all HUD panels). It uses `World->GetGameViewport()` -- NEVER `GEngine->GameViewport` (multiplayer-unsafe global singleton).

```cpp
void UZoneTransitionSubsystem::ShowLoadingOverlay(const FString& StatusText)
{
    if (bLoadingShown) return;

    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    LoadingWidget =
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.f))  // Near-black, fully opaque
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(StatusText))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.90f, 0.78f, 1.f)))
            .ShadowOffset(FVector2D(2, 2))
            .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f))
        ];

    LoadingOverlay = SNew(SWeakWidget).PossiblyNullContent(LoadingWidget);
    ViewportClient->AddViewportWidgetContent(LoadingOverlay.ToSharedRef(), 50);  // Z=50
    bLoadingShown = true;
}

void UZoneTransitionSubsystem::HideLoadingOverlay()
{
    if (!bLoadingShown) return;

    UWorld* World = GetWorld();
    if (World)
    {
        UGameViewportClient* ViewportClient = World->GetGameViewport();
        if (ViewportClient && LoadingOverlay.IsValid())
        {
            ViewportClient->RemoveViewportWidgetContent(LoadingOverlay.ToSharedRef());
        }
    }

    LoadingWidget.Reset();
    LoadingOverlay.Reset();
    bLoadingShown = false;
}
```

### 3.7 Error Handling

**Invalid zone/warp:** Server emits `zone:error` with a message. Client logs it. No transition occurs.

**Disconnection during transition:** The `bIsZoneTransitioning` flag in GameInstance persists. On reconnect, `CheckTransitionComplete` resumes polling. If the socket never reconnects, `ForceCompleteTransition` fires after 10 seconds.

**Missing pawn:** If the Level Blueprint fails to spawn `BP_MMOCharacter` (e.g., level was created from scratch instead of duplicated), `ForceCompleteTransition` fires after 10 seconds and completes the zone state update without a pawn teleport, logging an error with diagnostic information.

```cpp
void UZoneTransitionSubsystem::ForceCompleteTransition()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
    if (!GI) return;

    // Update zone state even without a pawn
    GI->CurrentZoneName = GI->PendingZoneName;
    CurrentZoneName = GI->CurrentZoneName;
    GI->bIsZoneTransitioning = false;

    // Emit zone:ready so the server sends enemies/players
    if (CachedSIOComponent.IsValid())
    {
        TSharedPtr<FJsonObject> ReadyPayload = MakeShareable(new FJsonObject());
        ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
        CachedSIOComponent->EmitNative(TEXT("zone:ready"), ReadyPayload);
    }

    World->GetTimerManager().ClearTimer(TransitionCheckTimer);
    World->GetTimerManager().SetTimerForNextTick([this]() { HideLoadingOverlay(); });

    UE_LOG(LogZoneTransition, Warning,
        TEXT("Zone transition force-completed -- now in %s (no pawn teleport)"), *CurrentZoneName);
}
```

### 3.8 Login Direct-Load (No Default Redirect)

On login, the character's saved zone is loaded directly instead of defaulting to L_PrtSouth. The REST `/api/characters` response includes `zone_name` and `level_name` per character. `FCharacterData` has `ZoneName` and `LevelName` fields.

`LoginFlowSubsystem::OnPlayCharacter` sets all pending zone state:

```cpp
void ULoginFlowSubsystem::OnPlayCharacter(const FCharacterData& Character)
{
    UMMOGameInstance* GI = GetGI();
    if (!GI) return;

    GI->SelectCharacter(Character.CharacterId);

    // Set the correct level/zone from the character's saved data
    GI->PendingLevelName = Character.LevelName;
    GI->PendingZoneName = Character.ZoneName;
    GI->CurrentZoneName = Character.ZoneName;
    GI->PendingSpawnLocation = FVector(Character.X, Character.Y, Character.Z);
    GI->bIsZoneTransitioning = true;

    TransitionTo(ELoginFlowState::EnteringWorld);
}
```

In the `EnteringWorld` state, the subsystem opens the pending level:

```cpp
UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
FString LevelName = TEXT("L_PrtSouth");  // Fallback only if no saved zone
if (GI && !GI->PendingLevelName.IsEmpty())
{
    LevelName = GI->PendingLevelName;
}
UGameplayStatics::OpenLevel(GetWorld(), *LevelName);
```

---

## 4. Warp Portal System

### 4.1 `AWarpPortal` -- Overlap Trigger Actor

**Files:** `client/SabriMMO/Source/SabriMMO/WarpPortal.h` / `.cpp`

`AWarpPortal` is a C++ actor with a sphere trigger component. When the local player pawn overlaps it, the actor calls `ZoneTransitionSubsystem::RequestWarp()` which emits `zone:warp` to the server.

```cpp
// WarpPortal.h
UCLASS()
class SABRIMMO_API AWarpPortal : public AActor
{
    GENERATED_BODY()

public:
    AWarpPortal();

    /** Must match a warpId in the server's ZONE_REGISTRY for the current zone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Portal")
    FString WarpId;

    /** Radius of the overlap trigger (UE units). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Portal")
    float TriggerRadius = 200.f;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USphereComponent* TriggerComp;

    /** Niagara particle effect for the portal visual. */
    UPROPERTY(VisibleAnywhere, Category = "VFX")
    UNiagaraComponent* PortalEffect;

    /** Niagara system asset to use for portal VFX. Assign in Blueprint or default. */
    UPROPERTY(EditAnywhere, Category = "VFX")
    UNiagaraSystem* PortalVFXSystem;

private:
    double LastWarpTime = 0.0;
    void TryActivatePortalVFX();
    FTimerHandle VFXRetryTimer;
    int32 VFXRetryCount = 0;
    UPROPERTY()
    TWeakObjectPtr<UNiagaraComponent> SpawnedPortalVFX;
};
```

### 4.2 Constructor

```cpp
AWarpPortal::AWarpPortal()
{
    PrimaryActorTick.bCanEverTick = false;

    // Sphere trigger for overlap detection
    TriggerComp = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerComp"));
    TriggerComp->InitSphereRadius(TriggerRadius);
    TriggerComp->SetCollisionProfileName(TEXT("OverlapAll"));
    TriggerComp->SetGenerateOverlapEvents(true);
    RootComponent = TriggerComp;

    // Niagara VFX component (auto-deactivated until assigned in BeginPlay)
    PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
    PortalEffect->SetupAttachment(RootComponent);
    PortalEffect->SetAutoActivate(false);
}
```

### 4.3 BeginPlay and VFX Initialization

```cpp
void AWarpPortal::BeginPlay()
{
    Super::BeginPlay();

    // Update radius in case it was edited per-instance
    TriggerComp->SetSphereRadius(TriggerRadius);
    TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);

    // Activate portal VFX
    if (PortalVFXSystem && PortalEffect)
    {
        // Explicitly assigned Niagara system -- use it directly
        PortalEffect->SetAsset(PortalVFXSystem);
        PortalEffect->Activate(true);
    }
    else
    {
        // Deferred: wait for SkillVFXSubsystem to load Niagara assets
        VFXRetryCount = 0;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(VFXRetryTimer,
                FTimerDelegate::CreateUObject(this, &AWarpPortal::TryActivatePortalVFX),
                1.0f, true);  // Retry every 1s
        }
    }
}

void AWarpPortal::TryActivatePortalVFX()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (++VFXRetryCount > 10)
    {
        World->GetTimerManager().ClearTimer(VFXRetryTimer);
        return;  // Give up after 10 seconds
    }

    USkillVFXSubsystem* VFXSub = World->GetSubsystem<USkillVFXSubsystem>();
    if (!VFXSub) return;

    UNiagaraComponent* Comp = VFXSub->SpawnLoopingPortalEffect(GetActorLocation());
    if (Comp)
    {
        SpawnedPortalVFX = Comp;
        World->GetTimerManager().ClearTimer(VFXRetryTimer);
    }
}
```

### 4.4 Overlap Detection

```cpp
void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // Only react to the local player pawn
    if (!OtherActor) return;
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || PC->GetPawn() != OtherActor) return;

    // 1-second spam guard (prevents rapid re-triggers)
    const double Now = FPlatformTime::Seconds();
    if (Now - LastWarpTime < 1.0) return;
    LastWarpTime = Now;

    // Request warp through the zone transition subsystem
    if (UZoneTransitionSubsystem* ZoneSub = World->GetSubsystem<UZoneTransitionSubsystem>())
    {
        ZoneSub->RequestWarp(WarpId);
    }
}
```

### 4.5 Cleanup

```cpp
void AWarpPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(VFXRetryTimer);
    }

    // Destroy the Niagara portal VFX to prevent orphaned components
    if (SpawnedPortalVFX.IsValid())
    {
        SpawnedPortalVFX->DeactivateImmediate();
        SpawnedPortalVFX->DestroyComponent();
        SpawnedPortalVFX = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}
```

### 4.6 Server-Side Warp Validation

The server performs these checks before allowing a warp:

1. **Player exists** in `connectedPlayers`
2. **Current zone exists** in `ZONE_REGISTRY`
3. **Warp exists** in the current zone's `warps` array (matching `warpId`)
4. **Proximity check:** Player's last known position must be within `radius * 2` of the warp position
5. **Destination zone exists** in `ZONE_REGISTRY`

```javascript
socket.on('zone:warp', async (data) => {
    const { warpId } = data;
    if (!warpId) {
        socket.emit('zone:error', { message: 'Missing warpId' });
        return;
    }

    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) {
        socket.emit('zone:error', { message: 'Player not found' });
        return;
    }

    const { characterId, player } = playerInfo;
    const oldZone = player.zone || 'prontera_south';
    const oldZoneData = getZone(oldZone);
    if (!oldZoneData) {
        socket.emit('zone:error', { message: 'Current zone not found' });
        return;
    }

    const warp = oldZoneData.warps.find(w => w.id === warpId);
    if (!warp) {
        socket.emit('zone:error', { message: `Warp '${warpId}' not found in ${oldZone}` });
        return;
    }

    // Proximity validation (generous: radius * 2)
    const maxWarpDist = (warp.radius || 200) * 2;
    const px = player.lastX || 0, py = player.lastY || 0;
    const warpDist = Math.sqrt((px - warp.x) ** 2 + (py - warp.y) ** 2);
    if (warpDist > maxWarpDist) {
        socket.emit('zone:error', { message: 'Too far from warp portal' });
        return;
    }

    const newZone = warp.destZone;
    const newZoneData = getZone(newZone);
    if (!newZoneData) {
        socket.emit('zone:error', { message: `Destination zone '${newZone}' not found` });
        return;
    }

    // ... proceed with zone transition (see Section 10.2)
});
```

### 4.7 Bidirectional Warps

Warps are always defined as pairs. Each zone defines its own exit warps, and the destination zones define their own entry warps:

```javascript
// In prontera:
warps: [{ id: 'prt_south_exit', ..., destZone: 'prontera_south', destX: 1330, ... }]

// In prontera_south:
warps: [{ id: 'prtsouth_to_prt', ..., destZone: 'prontera', destX: -240, destY: -1700, ... }]
```

### 4.8 Map-Edge Warps (Field Transitions)

Field-to-field warps are placed at the edges of the playable area. The trigger radius should match the warp registration:

```javascript
// Edge warp: player walks to the east edge of the field
{
    id: 'prtsouth_to_prt',
    x: 1740, y: 0, z: 0,       // Eastern map edge
    radius: 200,                 // Wide trigger strip
    destZone: 'prontera',
    destX: -240, destY: -1700, destZ: 590  // South gate of Prontera town
}
```

Place the `AWarpPortal` as a "strip" along the map edge. The sphere trigger radius of 200 units creates an invisible wall-like trigger zone. Players walking into it are warped to the adjacent zone.

---

## 5. Kafra NPC System

### 5.1 `AKafraNPC` -- Clickable NPC Actor

**Files:** `client/SabriMMO/Source/SabriMMO/KafraNPC.h` / `.cpp`

`AKafraNPC` is a C++ actor with a capsule collider (blocks player movement) and a visible mesh component (responds to trace for click detection).

```cpp
// KafraNPC.h
UCLASS()
class SABRIMMO_API AKafraNPC : public AActor
{
    GENERATED_BODY()

public:
    AKafraNPC();

    /** Must match a kafraId in the server's ZONE_REGISTRY for the current zone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
    FString KafraId;

    /** Display name shown above the NPC's head. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
    FString NPCDisplayName = TEXT("Kafra Employee");

    /** How close (UE units) the player must be to interact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
    float InteractionRadius = 300.f;

    /** Called from C++ when player clicks this NPC. Opens the Kafra UI. */
    UFUNCTION(BlueprintCallable, Category = "Kafra NPC")
    void Interact();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UCapsuleComponent* CapsuleComp;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComp;

private:
    double LastInteractTime = 0.0;
};
```

### 5.2 Constructor -- Collision Setup

The capsule blocks movement (Pawn collision profile) while the mesh blocks only visibility traces (for click detection):

```cpp
AKafraNPC::AKafraNPC()
{
    PrimaryActorTick.bCanEverTick = false;

    // Root capsule -- blocks player movement
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->InitCapsuleSize(42.f, 96.f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
    CapsuleComp->SetSimulatePhysics(false);
    CapsuleComp->SetEnableGravity(false);
    RootComponent = CapsuleComp;

    // Visual mesh -- trace-only collision for click detection
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // Default placeholder mesh (replace with actual Kafra model later)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        MeshComp->SetStaticMesh(CylinderMesh.Object);
    }
    MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
}
```

### 5.3 Interact -- Range-Checked Service Opening

```cpp
void AKafraNPC::Interact()
{
    // Spam guard (IA_Attack fires every frame while mouse is held)
    const double Now = FPlatformTime::Seconds();
    if (Now - LastInteractTime < 1.0) return;
    LastInteractTime = Now;

    UWorld* World = GetWorld();
    if (!World) return;

    // Range check -- player must be within InteractionRadius
    APlayerController* PC = World->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), GetActorLocation());
        if (Distance > InteractionRadius) return;
    }

    // Open Kafra dialog via KafraSubsystem
    if (UKafraSubsystem* KafraSub = World->GetSubsystem<UKafraSubsystem>())
    {
        if (!KafraSub->IsWidgetVisible())
        {
            KafraSub->RequestOpenKafra(KafraId);
        }
    }
}
```

### 5.4 `UKafraSubsystem` -- Service Management

**Files:** `client/SabriMMO/Source/SabriMMO/UI/KafraSubsystem.h` / `.cpp`

`UKafraSubsystem` is a `UWorldSubsystem` that manages Kafra service state, wraps Socket.io events, and controls the `SKafraWidget` lifecycle.

```cpp
// KafraSubsystem.h
USTRUCT()
struct FKafraDestination
{
    GENERATED_BODY()
    FString ZoneName;
    FString DisplayName;
    int32 Cost = 0;
};

UCLASS()
class SABRIMMO_API UKafraSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Kafra state (read by widget)
    FString CurrentKafraId;
    FString KafraName;
    TArray<FKafraDestination> Destinations;
    int32 PlayerZuzucoin = 0;
    FString CurrentSaveMap;
    FString StatusMessage;
    double StatusExpireTime = 0.0;

    // Public API
    void RequestOpenKafra(const FString& KafraId);
    void RequestSave();
    void RequestTeleport(const FString& DestZone);
    void CloseKafra();

    // Widget lifecycle
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

    // ...
};
```

### 5.5 Socket Events

The subsystem wraps four events:

| Event | Direction | Payload |
|-------|-----------|---------|
| `kafra:open` | Client -> Server | `{ kafraId }` |
| `kafra:data` | Server -> Client | `{ kafraId, kafraName, currentSaveMap, playerZuzucoin, destinations[] }` |
| `kafra:save` | Client -> Server | `{}` |
| `kafra:saved` | Server -> Client | `{ saveMap }` |
| `kafra:teleport` | Client -> Server | `{ kafraId, destZone }` |
| `kafra:teleported` | Server -> Client | `{ destZone, remainingZuzucoin }` |
| `kafra:error` | Server -> Client | `{ message }` |

### 5.6 Service Flow: Save Point

```
Player clicks Kafra NPC
    -> Interact() -> KafraSubsystem::RequestOpenKafra(kafraId)
    -> Emits kafra:open { kafraId }

Server receives kafra:open
    -> Validates kafraId exists in current zone's kafraNpcs
    -> Reads player's zuzucoin and current save_map from DB
    -> Emits kafra:data { kafraId, kafraName, currentSaveMap, playerZuzucoin, destinations[] }

Client receives kafra:data
    -> HandleKafraData() parses payload, populates Destinations array
    -> ShowWidget() creates SKafraWidget, adds to viewport at Z=19
    -> Disables player movement (DisableMovement())

Player clicks "Save" button
    -> KafraSubsystem::RequestSave()
    -> Emits kafra:save {}

Server receives kafra:save
    -> Updates characters SET save_map = current zone, save_x/y/z = player position
    -> Emits kafra:saved { saveMap }

Client receives kafra:saved
    -> HandleKafraSaved() updates CurrentSaveMap
    -> Sets StatusMessage = "Save point set!" (3-second display)
```

### 5.7 Service Flow: Teleport

```
Player clicks teleport destination
    -> KafraSubsystem::RequestTeleport(destZone)
    -> Emits kafra:teleport { kafraId, destZone }

Server receives kafra:teleport
    -> Validates destination exists in kafra's destinations[]
    -> Checks playerZuzucoin >= cost
    -> Deducts cost from DB
    -> Emits kafra:teleported { destZone, remainingZuzucoin }
    -> Performs zone transition (same as warp: leave room, join room, etc.)
    -> Emits zone:change { zone, displayName, levelName, x, y, z, flags }

Client receives kafra:teleported
    -> HandleKafraTeleported() updates PlayerZuzucoin
    -> CloseKafra() (hides widget, clears state)

Client receives zone:change
    -> HandleZoneChange() starts full zone transition flow (see Section 3.4)
```

### 5.8 Widget Lifecycle

The widget disables player movement while open and re-enables it on close:

```cpp
void UKafraSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    KafraWidget = SNew(SKafraWidget).Subsystem(this);

    AlignmentWrapper =
        SNew(SBox)
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [
            KafraWidget.ToSharedRef()
        ];

    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
    ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 19);  // Z=19
    bWidgetAdded = true;

    // Lock player movement
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (ACharacter* Char = PC->GetCharacter())
        {
            if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
            {
                MovComp->DisableMovement();
            }
        }
    }
}

void UKafraSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    // Remove from viewport...

    // Unlock player movement
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ACharacter* Char = PC->GetCharacter())
            {
                if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
                {
                    MovComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
    }
}
```

### 5.9 Server-Side Kafra Implementation

Complete server handler for Kafra events:

```javascript
// ============================================================
// KAFRA EVENTS
// ============================================================

socket.on('kafra:open', async (data) => {
    const { kafraId } = data;
    if (!kafraId) {
        socket.emit('kafra:error', { message: 'Missing kafraId' });
        return;
    }

    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    const zone = player.zone || 'prontera_south';
    const zoneData = getZone(zone);
    if (!zoneData) {
        socket.emit('kafra:error', { message: 'Zone not found' });
        return;
    }

    // Find kafra NPC in current zone
    const kafra = zoneData.kafraNpcs.find(k => k.id === kafraId);
    if (!kafra) {
        socket.emit('kafra:error', { message: 'Kafra NPC not found in this zone' });
        return;
    }

    // Fetch player zuzucoin and save map from DB
    let zuzucoin = 0, saveMap = 'prontera';
    try {
        const result = await pool.query(
            'SELECT zuzucoin, save_map FROM characters WHERE character_id = $1',
            [characterId]
        );
        if (result.rows.length > 0) {
            zuzucoin = result.rows[0].zuzucoin || 0;
            saveMap = result.rows[0].save_map || 'prontera';
        }
    } catch (err) {
        logger.error(`Kafra DB error: ${err.message}`);
    }

    socket.emit('kafra:data', {
        kafraId: kafra.id,
        kafraName: kafra.name,
        currentSaveMap: saveMap,
        playerZuzucoin: zuzucoin,
        destinations: kafra.destinations
    });
});

socket.on('kafra:save', async () => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    const zone = player.zone || 'prontera_south';

    try {
        await pool.query(
            'UPDATE characters SET save_map = $1, save_x = $2, save_y = $3, save_z = $4 WHERE character_id = $5',
            [zone, player.lastX || 0, player.lastY || 0, player.lastZ || 300, characterId]
        );
        socket.emit('kafra:saved', { saveMap: zone });
    } catch (err) {
        socket.emit('kafra:error', { message: 'Failed to save' });
    }
});

socket.on('kafra:teleport', async (data) => {
    const { kafraId, destZone } = data;
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    const zone = player.zone || 'prontera_south';
    const zoneData = getZone(zone);
    const kafra = zoneData?.kafraNpcs?.find(k => k.id === kafraId);
    if (!kafra) {
        socket.emit('kafra:error', { message: 'Kafra not found' });
        return;
    }

    const dest = kafra.destinations.find(d => d.zone === destZone);
    if (!dest) {
        socket.emit('kafra:error', { message: 'Invalid destination' });
        return;
    }

    // Check zeny
    let zuzucoin = 0;
    try {
        const r = await pool.query('SELECT zuzucoin FROM characters WHERE character_id = $1', [characterId]);
        zuzucoin = r.rows[0]?.zuzucoin || 0;
    } catch (err) {
        socket.emit('kafra:error', { message: 'Database error' });
        return;
    }

    if (zuzucoin < dest.cost) {
        socket.emit('kafra:error', { message: `Not enough Zeny (need ${dest.cost}, have ${zuzucoin})` });
        return;
    }

    // Deduct cost
    try {
        await pool.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2',
            [dest.cost, characterId]);
    } catch (err) {
        socket.emit('kafra:error', { message: 'Payment failed' });
        return;
    }

    socket.emit('kafra:teleported', {
        destZone,
        remainingZuzucoin: zuzucoin - dest.cost
    });

    // Perform zone transition (reuses warp logic)
    const newZoneData = getZone(destZone);
    if (!newZoneData) {
        socket.emit('zone:error', { message: 'Destination zone not found' });
        return;
    }

    // ... same zone transition logic as zone:warp handler (leave old room, join new room, etc.)
    // Emit zone:change to client
    socket.emit('zone:change', {
        zone: destZone,
        displayName: newZoneData.displayName,
        levelName: newZoneData.levelName,
        x: newZoneData.defaultSpawn.x,
        y: newZoneData.defaultSpawn.y,
        z: newZoneData.defaultSpawn.z,
        flags: newZoneData.flags,
        reason: 'kafra_teleport'
    });
});
```

### 5.10 Future Services (Storage, Cart Rental)

These follow the same pattern:

| Service | Event Flow | DB Impact |
|---------|------------|-----------|
| **Storage** | `kafra:storage_open` -> server sends storage items -> `kafra:storage_data` | New `character_storage` table |
| **Cart Rental** | `kafra:cart_rent` -> server checks zeny -> `kafra:cart_rented` | Merchant-class only, status flag on character |

---

## 6. Landscape / Terrain Creation

### 6.1 UE5 Landscape Tool Basics

For zones requiring natural terrain (fields, forests, mountains), UE5's Landscape system provides scalable heightmap-based terrain.

**Recommended terrain sizes by zone type:**

| Zone Type | Landscape Size | Components | Quads/Section | Notes |
|-----------|---------------|------------|---------------|-------|
| Town (small) | 1009x1009 | 1x1 sections, 63x63 quads | 63 | Small area, flat with minor features |
| Field (medium) | 2017x2017 | 2x2 sections, 63x63 quads | 63 | Standard outdoor zone |
| Large Field | 4033x4033 | 4x4 sections, 63x63 quads | 63 | Wide-open RO field equivalent |
| Dungeon | N/A | Use static mesh geometry | N/A | Indoor zones do not use Landscape |

**Creating a Landscape via code is not practical** -- the Landscape tool requires Editor interaction for heightmap painting and layer assignment. However, flat landscapes can be created programmatically:

**Alternative for flat terrain (used in current zones):**
Instead of Landscape, place a scaled Cube static mesh as a ground plane:

```
// Flat ground plane (programmatic via unrealMCP or Level Editor)
Location: (0, 0, 0)
Mesh: /Engine/BasicShapes/Cube.Cube
Scale: (60, 60, 0.5)  // 6000x6000 UE units, 50 units tall
Material: Earth/Grass material
```

This approach is used for quick prototyping and blockout. Convert to Landscape later for production.

### 6.2 Terrain Material (Multi-Layer Blend)

UE5 landscapes support layer-blended materials. For RO-inspired zones:

**Prontera Fields (grassland):**
- Layer 1: Green grass (base)
- Layer 2: Dirt/path (painted along walkways)
- Layer 3: Rock (painted on hillsides)

**Sograt Desert (Morroc area):**
- Layer 1: Sand (base)
- Layer 2: Red sandstone
- Layer 3: Cracked dry earth

**Payon Forest:**
- Layer 1: Dark earth with leaf litter (base)
- Layer 2: Moss-covered ground
- Layer 3: Tree roots/bark ground

**Snow zones (Rachel/Lutie):**
- Layer 1: Snow (base)
- Layer 2: Ice/frozen ground
- Layer 3: Exposed rock

Material creation requires the UE5 Material Editor and is outside the scope of C++ automation. Use Quixel Bridge or Megascans for high-quality terrain textures.

### 6.3 Foliage Placement

UE5's Foliage tool paints instanced meshes (grass, flowers, bushes, trees) across the landscape.

**Performance guidelines:**
- Maximum instanced foliage count: ~50,000 per zone (for MMO-grade performance)
- Use LODs (Level of Detail) on all foliage meshes
- Grass/ground cover: cull distance 5000-8000 UE units
- Trees: cull distance 15000+ UE units
- Enable nanite for tree meshes if available

### 6.4 Water Bodies

UE5.1+ provides the Water system (Water Body actors):
- `WaterBodyRiver` -- for rivers, streams
- `WaterBodyLake` -- for ponds, lakes
- `WaterBodyOcean` -- for coastal zones (Alberta, Comodo)

Water bodies require the Water plugin enabled in the project. For simpler implementations, a translucent plane mesh with a water material works as a placeholder.

### 6.5 Performance Considerations

| Setting | Recommendation | Reason |
|---------|---------------|--------|
| View distance | 20000 UE units | RO zones are relatively small |
| Shadow distance | 10000 UE units | Reduce shadow map overhead |
| Nanite | Enable for static meshes | Free LOD for buildings/terrain |
| Lumen | Disable in favor of baked lighting | MMO needs consistent FPS |
| Foliage density | Medium (LOD aggressive) | Prioritize frame rate |
| Texture streaming | Enable with pool 1024MB | Prevent texture pop-in |

---

## 7. Environment Art Placement

### 7.1 Building Placement

RO towns have distinctive architectural themes. Each building is a Static Mesh Actor placed in the level:

**Prontera (Medieval European):**
- White stone castle walls
- Gothic cathedral
- Cobblestone streets
- Merchant stalls with wooden canopies
- Fountain in town center

**Geffen (Magic Tower):**
- Central tower (Geffen Tower dungeon entrance)
- Stone walls with magical rune inscriptions
- Floating crystal lights
- Library/academy buildings

**Payon (Korean/East Asian):**
- Bamboo fences and gates
- Thatched-roof houses
- Stone lanterns
- Bridge over stream

### 7.2 Prop Placement

Props add life to zones:
- **Towns:** Barrels, crates, market stalls, banners, signs, benches
- **Fields:** Rock formations, fallen logs, bushes, flowers, signposts at crossroads
- **Dungeons:** Torch holders, chains, broken furniture, bones, cobwebs

### 7.3 Lighting Setup Per Zone Type

**Town (daytime):**
```
Directional Light: Intensity 3.0, Color (1.0, 0.95, 0.85), Rotation (Pitch -45, Yaw 170)
Sky Atmosphere: Default settings
Sky Light: Intensity 1.0, Real Time Capture enabled
Exponential Height Fog: Density 0.002, Start Distance 1000
```

**Field (outdoor):**
```
Directional Light: Intensity 4.0, Color (1.0, 0.98, 0.90), Rotation varies by zone
Sky Atmosphere: Default
Sky Light: Intensity 0.8
Exponential Height Fog: Density 0.005, Fog Height Falloff 0.5
```

**Dungeon (underground):**
```
NO Directional Light, NO Sky
Point Lights: Intensity 500-2000, Radius 300-800
    Colors by theme:
    - Prontera Culvert: Warm orange (0.9, 0.6, 0.3)
    - Payon Cave: Blue-green (0.3, 0.6, 0.8)
    - Geffen Tower: Purple (0.5, 0.2, 0.8)
    - Bio Lab: Clinical white (0.9, 0.95, 1.0)
Spotlight: Near entrances, boss rooms
```

**Night zones (Niflheim):**
```
Directional Light: Very low intensity 0.3, Blue-white color (0.6, 0.7, 1.0)
Moon Light: Secondary directional, Intensity 0.1
Sky Light: Intensity 0.2, Tint (0.5, 0.5, 0.7)
Exponential Height Fog: Density 0.01, Fog Inscattering Color dark purple
```

### 7.4 Skybox / Atmosphere

UE5's Sky Atmosphere component provides physically-based atmospheric scattering:
- **Default outdoor:** Standard Sky Atmosphere + Directional Light = realistic blue sky
- **Desert (Morroc):** Increase Rayleigh scattering to warm the sky
- **Snowy (Rachel):** Low sun angle, high fog density, overcast
- **Volcanic (Veins):** Red-tinted atmosphere, heavy fog, ember particles

### 7.5 Post-Processing Volumes

Use Post-Process Volumes for zone-specific visual treatments:

```
// Town (bright, cheerful)
PPV: Auto Exposure Min/Max 1.0-3.0, Bloom Intensity 0.3, Vignette 0.2

// Dungeon (dark, moody)
PPV: Auto Exposure Min/Max 0.5-1.5, Bloom Intensity 0.1, Vignette 0.5
     Color Grading: Desaturate slightly, shift toward zone's color theme

// PvP Arena (high contrast)
PPV: Auto Exposure 2.0-4.0, Bloom 0.5, Sharpen 0.3
```

---

## 8. Minimap Data

### 8.1 Minimap Texture Per Zone

Each zone requires a top-down minimap texture stored as a UI asset:

```
Content/SabriMMO/UI/Minimaps/
    T_Minimap_Prontera.png        -- 512x512 or 1024x1024
    T_Minimap_PrtSouth.png
    T_Minimap_PrtNorth.png
    T_Minimap_PrtDungeon01.png
```

Generate minimap textures by:
1. Top-down screenshot from the UE5 Editor (orthographic camera, high altitude)
2. Post-process in image editor: add roads, landmarks, zone border
3. Import as Texture2D with UI compression settings

### 8.2 Player Position Mapping

Convert world coordinates to minimap texture coordinates:

```cpp
// MinimapSubsystem.h
USTRUCT()
struct FMinimapBounds
{
    GENERATED_BODY()

    FVector2D WorldMin;     // Bottom-left corner of zone in world space
    FVector2D WorldMax;     // Top-right corner of zone in world space
    FVector2D TextureSize;  // Minimap texture dimensions (e.g., 512x512)
};

// Convert world position to minimap UV
FVector2D WorldToMinimapUV(const FVector& WorldPos, const FMinimapBounds& Bounds)
{
    float U = (WorldPos.X - Bounds.WorldMin.X) / (Bounds.WorldMax.X - Bounds.WorldMin.X);
    float V = (WorldPos.Y - Bounds.WorldMin.Y) / (Bounds.WorldMax.Y - Bounds.WorldMin.Y);
    return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

// Convert UV to pixel position on minimap widget
FVector2D MinimapUVToPixel(const FVector2D& UV, const FVector2D& WidgetSize)
{
    return FVector2D(UV.X * WidgetSize.X, (1.0f - UV.Y) * WidgetSize.Y);  // Flip Y for screen space
}
```

### 8.3 Minimap Bounds Configuration

Define bounds per zone in a data structure (can live in the zone registry or a client-side config):

```cpp
// Example minimap bounds (matching ro_zone_data.js zones)
static TMap<FString, FMinimapBounds> MinimapConfigs = {
    { TEXT("prontera"),       { FVector2D(-3500, -3000), FVector2D(3500, 8000),  FVector2D(512, 512) } },
    { TEXT("prontera_south"), { FVector2D(-2000, -2000), FVector2D(2000, 2000),  FVector2D(512, 512) } },
    { TEXT("prontera_north"), { FVector2D(-2000, -2000), FVector2D(2000, 2000),  FVector2D(512, 512) } },
    { TEXT("prt_dungeon_01"), { FVector2D(-2000, -2000), FVector2D(2000, 2000),  FVector2D(512, 512) } },
};
```

### 8.4 NPC/Warp Portal Markers

Overlay icons on the minimap at the UV positions of NPCs and warp portals:

```cpp
// During minimap render (OnPaint):
for (const auto& Warp : CurrentZoneWarps)
{
    FVector2D UV = WorldToMinimapUV(FVector(Warp.X, Warp.Y, 0), Bounds);
    FVector2D Pixel = MinimapUVToPixel(UV, WidgetSize);

    // Draw warp portal icon (blue arrow or portal symbol)
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
        AllottedGeometry.ToPaintGeometry(FVector2D(16, 16), FSlateLayoutTransform(Pixel - FVector2D(8, 8))),
        WarpIconBrush, ESlateDrawEffect::None, FLinearColor::Blue);
}

for (const auto& NPC : CurrentZoneNPCs)
{
    FVector2D UV = WorldToMinimapUV(FVector(NPC.X, NPC.Y, 0), Bounds);
    FVector2D Pixel = MinimapUVToPixel(UV, WidgetSize);

    // Draw NPC icon (green dot)
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
        AllottedGeometry.ToPaintGeometry(FVector2D(12, 12), FSlateLayoutTransform(Pixel - FVector2D(6, 6))),
        NPCIconBrush, ESlateDrawEffect::None, FLinearColor::Green);
}
```

### 8.5 Future: MinimapSubsystem

A dedicated `UMinimapSubsystem` (UWorldSubsystem) would:
1. Load minimap texture for the current zone on `OnWorldBeginPlay`
2. Track player position via `GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation()`
3. Render `SMinimapWidget` as an overlay (Z=5 or lower)
4. Show player arrow, party members, NPC dots, warp portal arrows
5. Support toggle between corner minimap and fullscreen map (M key)

---

## 9. Map Properties / Flags

### 9.1 Map Flags System

RO Classic uses map flags to control what actions are allowed on each map. Sabri_MMO implements the core flags both server-side (enforcement) and client-side (UI display/restriction).

**Implemented flags:**

| Flag | Type | Description | Server Enforcement | Client Display |
|------|------|-------------|-------------------|----------------|
| `town` | bool | Safe zone, no monster spawns | Skips enemy AI tick | Town indicator in HUD |
| `noteleport` | bool | Blocks Fly Wing, Teleport skill | Rejects `inventory:use` for Fly Wing (1029) | Grays out Fly Wing in inventory |
| `noreturn` | bool | Blocks Butterfly Wing, Return skill | Rejects `inventory:use` for Butterfly Wing (1028) | Grays out Butterfly Wing in inventory |
| `nosave` | bool | Cannot set save point here | Rejects `kafra:save` | Disables "Save" in Kafra dialog |
| `pvp` | bool | Enables Player vs Player combat | Allows player-target skills | "PvP Zone" indicator in HUD |
| `indoor` | bool | Indoor lighting model | No gameplay effect | Client uses point lights only |

**Planned flags (not yet implemented):**

| Flag | Description |
|------|-------------|
| `gvg` | Enables Guild vs Guild combat |
| `gvg_castle` | WoE castle special rules |
| `nowarp` | Blocks @warp commands |
| `nowarpto` | Blocks warping TO this map |
| `nomemo` | Blocks /memo (Warp Portal memory) |
| `nopenalty` | No EXP/Zeny loss on death |
| `noskill` | All skills disabled |
| `nodrop` | Cannot drop items |
| `notrade` | Cannot trade items |
| `novending` | Cannot open vend shop |

### 9.2 Flag Definition in Zone Registry

Flags are defined per zone in `ro_zone_data.js`:

```javascript
// Town (safe zone)
flags: {
    noteleport: false,
    noreturn: false,
    nosave: false,
    pvp: false,
    town: true,
    indoor: false
}

// Dungeon (restrictive)
flags: {
    noteleport: true,   // Fly Wing blocked
    noreturn: true,      // Butterfly Wing blocked
    nosave: true,        // Cannot save here
    pvp: false,
    town: false,
    indoor: true
}

// PvP Arena
flags: {
    noteleport: false,
    noreturn: true,
    nosave: true,
    pvp: true,
    town: false,
    indoor: false
}
```

### 9.3 Server-Side Flag Validation

The server checks flags when processing item use and skill casts:

```javascript
// In inventory:use handler (Fly Wing / Butterfly Wing)
socket.on('inventory:use', async (data) => {
    const { inventoryId } = data;
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    // ... look up item ...

    const zoneData = getZone(player.zone);

    // Fly Wing (item ID 1029)
    if (item.item_id === 1029) {
        if (zoneData?.flags?.noteleport) {
            socket.emit('inventory:error', { message: 'Cannot teleport in this zone' });
            return;
        }
        // ... perform random teleport within zone ...
        socket.emit('player:teleport', {
            characterId, x: randomX, y: randomY, z: randomZ,
            teleportType: 'fly_wing'
        });
    }

    // Butterfly Wing (item ID 1028)
    if (item.item_id === 1028) {
        if (zoneData?.flags?.noreturn) {
            socket.emit('inventory:error', { message: 'Cannot use return items in this zone' });
            return;
        }
        // ... teleport to save point (may trigger zone change) ...
    }
});
```

### 9.4 Client-Side Flag Display

`UZoneTransitionSubsystem` stores the current zone's flags as public booleans:

```cpp
// ZoneTransitionSubsystem.h
bool bNoTeleport = false;
bool bNoReturn = false;
bool bNoSave = false;
bool bIsTown = false;
```

These are updated whenever `zone:change` is received (see Section 3.4). Other subsystems can read them:

```cpp
// Example: Inventory subsystem checking if Fly Wing is usable
UZoneTransitionSubsystem* ZoneSub = GetWorld()->GetSubsystem<UZoneTransitionSubsystem>();
if (ZoneSub && ZoneSub->bNoTeleport)
{
    // Gray out Fly Wing, show "Cannot use in this zone" tooltip
}
```

### 9.5 Common Flag Combinations

| Zone Type | Flags Set |
|-----------|-----------|
| **Town** | `town=true` |
| **Field** | All false (open area) |
| **Dungeon** | `noteleport=true, noreturn=true, nosave=true, indoor=true` |
| **Boss Room** | `noteleport=true, noreturn=true, nosave=true, nowarpto=true, indoor=true` |
| **PvP Arena** | `pvp=true, noreturn=true, nosave=true` |
| **GvG Castle** | `gvg=true, gvg_castle=true, noreturn=true` |
| **Instance** | `noteleport=true, noreturn=true, nosave=true, nowarpto=true, nowarp=true` |

---

## 10. Server-Side Zone Management

### 10.1 Zone Registry Data Structure

The complete zone registry is defined in `server/src/ro_zone_data.js` (see Section 1.2). It exports:

```javascript
module.exports = { ZONE_REGISTRY, getZone, getAllEnemySpawns, getZoneNames };
```

Imported at the top of `server/src/index.js`:

```javascript
const { ZONE_REGISTRY, getZone, getAllEnemySpawns, getZoneNames } = require('./ro_zone_data');
```

### 10.2 Broadcasting Functions

All gameplay events are broadcast to zone-scoped Socket.io rooms:

```javascript
/**
 * Broadcast an event to ALL sockets in a zone (including sender).
 */
function broadcastToZone(zone, event, data) {
    io.to('zone:' + zone).emit(event, data);
}

/**
 * Broadcast an event to all sockets in a zone EXCEPT the sender.
 * Used for: player:moved, combat:health_update (don't echo back to sender)
 */
function broadcastToZoneExcept(socket, zone, event, data) {
    socket.to('zone:' + zone).emit(event, data);
}
```

**Usage throughout the server:**

```javascript
// Player movement -- broadcast to all others in same zone
broadcastToZoneExcept(socket, playerZone, 'player:moved', {
    characterId, characterName, x, y, z, health, maxHealth, timestamp: Date.now()
});

// Enemy death -- broadcast to entire zone (including killer)
broadcastToZone(enemyZone, 'enemy:death', {
    enemyId, killerCharacterId, killerName, enemyName, expReward
});

// Skill damage -- broadcast to entire zone
broadcastToZone(boltZone, 'skill:effect_damage', {
    casterId: characterId, skillId, targetId, isEnemy, damage, hitNumber, totalHits,
    targetX, targetY, targetZ
});
```

### 10.3 Monster Spawns Per Zone

Enemies are lazily spawned -- the server only instantiates enemies in a zone when the first player enters:

```javascript
// Track which zones have enemies spawned
const spawnedZones = new Set();

// In zone:warp handler (after room switch):
if (!spawnedZones.has(newZone)) {
    if (newZoneData.enemySpawns.length > 0) {
        logger.info(`[ZONE] First player in '${newZone}' -- spawning ${newZoneData.enemySpawns.length} enemies`);
        for (const spawn of newZoneData.enemySpawns) {
            spawnEnemy({ ...spawn, zone: newZone });
        }
    }
    spawnedZones.add(newZone);
}
```

Each `spawnEnemy()` call creates an enemy instance with:
- Template from `ro_monster_templates.js` (stats, AI code, loot table)
- Zone assignment (`enemy.zone = newZone`)
- Spawn position (`enemy.x, enemy.y, enemy.z`)
- Wander radius from the spawn config

### 10.4 NPC Registry Per Zone

NPCs are defined in the zone registry and are purely client-side actors. The server only needs to know about Kafra NPCs (for service validation). Other NPCs (tool dealers, job change, etc.) are planned to use a similar pattern:

```javascript
// Future: NPC registry in zone data
npcs: [
    {
        id: 'weapon_dealer_prt_1',
        type: 'shop',
        shopId: 1,
        name: 'Weapon Dealer',
        x: -600, y: -500, z: 300
    },
    {
        id: 'job_change_acolyte',
        type: 'job_change',
        jobClass: 'acolyte',
        name: 'Father Mareusis',
        x: 800, y: 2000, z: 300
    }
]
```

### 10.5 Zone Population Tracking

The server tracks zone populations for multiple purposes:

```javascript
/**
 * Get the player's current zone from in-memory state.
 */
function getPlayerZone(characterId) {
    const player = connectedPlayers.get(characterId);
    return player ? (player.zone || 'prontera_south') : 'prontera_south';
}

/**
 * Check if any player is in a specific zone (for AI optimization).
 */
function isZoneActive(zone) {
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone === zone) return true;
    }
    return false;
}

/**
 * Get all zones with at least one player (for AI tick).
 */
function getActiveZones() {
    const zones = new Set();
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone) zones.add(player.zone);
    }
    return zones;
}
```

**AI optimization:** The enemy AI tick loop only processes enemies in active zones:

```javascript
// In the 200ms AI tick loop:
const activeZones = getActiveZones();
for (const [eid, enemy] of enemies.entries()) {
    // Skip enemies in zones with no players
    if (!activeZones.has(enemy.zone)) continue;

    // ... process AI state machine for this enemy ...
}
```

### 10.6 Zone:Ready Handler -- Player Arrival

When a client finishes loading a new level and emits `zone:ready`, the server:

```javascript
socket.on('zone:ready', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    const zone = player.zone || 'prontera_south';

    // 1. Announce this player's arrival to others in the zone
    broadcastToZoneExcept(socket, zone, 'player:moved', {
        characterId,
        characterName: player.characterName,
        x: player.lastX || 0,
        y: player.lastY || 0,
        z: player.lastZ || 300,
        health: player.health,
        maxHealth: player.maxHealth,
        timestamp: Date.now()
    });

    // 2. Send this player's health to the zone
    broadcastToZoneExcept(socket, zone, 'combat:health_update', {
        characterId,
        health: player.health,
        maxHealth: player.maxHealth,
        mana: player.mana,
        maxMana: player.maxMana
    });

    // 3. Send all zone enemies to this client
    for (const [eid, enemy] of enemies.entries()) {
        if (!enemy.isDead && enemy.zone === zone) {
            socket.emit('enemy:spawn', {
                enemyId: eid,
                templateId: enemy.templateId,
                name: enemy.name,
                level: enemy.level,
                health: enemy.health,
                maxHealth: enemy.maxHealth,
                x: enemy.x,
                y: enemy.y,
                z: enemy.z
            });
        }
    }

    // 4. Send all other players in this zone to the arriving client
    for (const [cid, otherPlayer] of connectedPlayers.entries()) {
        if (cid !== characterId && otherPlayer.zone === zone) {
            socket.emit('player:moved', {
                characterId: cid,
                characterName: otherPlayer.characterName,
                x: otherPlayer.lastX || 0,
                y: otherPlayer.lastY || 0,
                z: otherPlayer.lastZ || 300,
                health: otherPlayer.health,
                maxHealth: otherPlayer.maxHealth,
                timestamp: Date.now()
            });
        }
    }

    // 5. Remove from transitioning set
    zoneTransitioning.delete(characterId);

    logger.info(`[ZONE] ${player.characterName} ready in '${zone}'`);
});
```

### 10.7 Disconnect Handler -- Zone Cleanup

When a player disconnects, the server:

```javascript
socket.on('disconnect', async () => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    const leftZone = player.zone || 'prontera_south';

    // Broadcast player:left to the zone
    broadcastToZone(leftZone, 'player:left', {
        characterId,
        characterName: player.characterName,
        reason: 'disconnect'
    });

    // Save position + zone to database
    try {
        await pool.query(
            'UPDATE characters SET x = $1, y = $2, z = $3, zone_name = $4 WHERE character_id = $5',
            [player.lastX || 0, player.lastY || 0, player.lastZ || 300, leftZone, characterId]
        );
    } catch (err) {
        logger.error(`Failed to save position on disconnect: ${err.message}`);
    }

    // Remove from combat/AI tracking
    autoAttackState.delete(characterId);
    activeCasts.delete(characterId);
    afterCastDelayEnd.delete(characterId);
    zoneTransitioning.delete(characterId);

    // Clear enemy aggro toward this player
    for (const [, enemy] of enemies.entries()) {
        if (enemy.zone === leftZone) {
            enemy.inCombatWith.delete(characterId);
            if (enemy.targetPlayerId === characterId) {
                enemy.targetPlayerId = null;
                // Pick next target or return to idle
            }
        }
    }

    connectedPlayers.delete(characterId);
});
```

### 10.8 Zone Weather Effects (Future)

Weather effects are defined per zone in the flags:

```javascript
flags: {
    // ... existing flags ...
    weather: 'rain',       // 'none' | 'rain' | 'snow' | 'fog' | 'leaves' | 'fireworks'
    weatherIntensity: 0.5  // 0.0 - 1.0
}
```

Client-side implementation:
1. `ZoneTransitionSubsystem` parses weather flags from `zone:change`
2. Spawns a weather Niagara system at the camera location
3. Updates position each frame to follow the camera
4. Destroys on zone transition

```cpp
// Future: Weather system in ZoneTransitionSubsystem
void UZoneTransitionSubsystem::ApplyZoneWeather(const FString& Weather, float Intensity)
{
    // Destroy existing weather
    if (ActiveWeatherComponent.IsValid())
    {
        ActiveWeatherComponent->DeactivateImmediate();
        ActiveWeatherComponent->DestroyComponent();
        ActiveWeatherComponent = nullptr;
    }

    if (Weather == TEXT("none") || Weather.IsEmpty()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Map weather type to Niagara system
    UNiagaraSystem* WeatherSystem = nullptr;
    if (Weather == TEXT("rain"))
    {
        WeatherSystem = LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/SabriMMO/VFX/Weather/NS_Rain.NS_Rain"));
    }
    else if (Weather == TEXT("snow"))
    {
        WeatherSystem = LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/SabriMMO/VFX/Weather/NS_Snow.NS_Snow"));
    }
    // ... more weather types ...

    if (WeatherSystem)
    {
        APlayerController* PC = World->GetFirstPlayerController();
        if (PC && PC->GetPawn())
        {
            FVector SpawnLoc = PC->GetPawn()->GetActorLocation() + FVector(0, 0, 1000);
            ActiveWeatherComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World, WeatherSystem, SpawnLoc, FRotator::ZeroRotator,
                FVector::OneVector, true, true, ENCPoolMethod::AutoRelease);
        }
    }
}
```

### 10.9 Database Schema for Zone System

```sql
-- Zone System Migration (database/migrations/add_zone_system.sql)

-- Zone tracking: which map the character is currently in
ALTER TABLE characters
ADD COLUMN IF NOT EXISTS zone_name VARCHAR(50) DEFAULT 'prontera_south';

-- Save point: where the character respawns on death / Butterfly Wing
ALTER TABLE characters
ADD COLUMN IF NOT EXISTS save_map VARCHAR(50) DEFAULT 'prontera',
ADD COLUMN IF NOT EXISTS save_x FLOAT DEFAULT 0,
ADD COLUMN IF NOT EXISTS save_y FLOAT DEFAULT 0,
ADD COLUMN IF NOT EXISTS save_z FLOAT DEFAULT 580;

-- Index for efficient zone-based queries
CREATE INDEX IF NOT EXISTS idx_characters_zone ON characters(zone_name);
```

The server auto-creates missing columns on startup, so new columns in future migrations will be picked up automatically.

### 10.10 Complete Zone:Warp Server Handler

For reference, the complete server-side warp handler with all combat cleanup, room switching, lazy enemy spawning, and DB persistence:

```javascript
socket.on('zone:warp', async (data) => {
    const { warpId } = data;
    if (!warpId) {
        socket.emit('zone:error', { message: 'Missing warpId' });
        return;
    }

    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) {
        socket.emit('zone:error', { message: 'Player not found' });
        return;
    }
    const { characterId, player } = playerInfo;
    const oldZone = player.zone || 'prontera_south';
    const oldZoneData = getZone(oldZone);
    if (!oldZoneData) {
        socket.emit('zone:error', { message: 'Current zone not found' });
        return;
    }

    // Find warp definition in current zone
    const warp = oldZoneData.warps.find(w => w.id === warpId);
    if (!warp) {
        socket.emit('zone:error', { message: `Warp '${warpId}' not found in ${oldZone}` });
        return;
    }

    // Proximity validation
    const maxWarpDist = (warp.radius || 200) * 2;
    const px = player.lastX || 0, py = player.lastY || 0;
    const warpDist = Math.sqrt((px - warp.x) ** 2 + (py - warp.y) ** 2);
    if (warpDist > maxWarpDist) {
        socket.emit('zone:error', { message: 'Too far from warp portal' });
        return;
    }

    const newZone = warp.destZone;
    const newZoneData = getZone(newZone);
    if (!newZoneData) {
        socket.emit('zone:error', { message: `Destination zone '${newZone}' not found` });
        return;
    }

    // Cancel active combat/casting
    autoAttackState.delete(characterId);
    activeCasts.delete(characterId);
    afterCastDelayEnd.delete(characterId);

    // Clear enemy aggro in old zone
    for (const [, enemy] of enemies.entries()) {
        if (enemy.zone === oldZone) {
            enemy.inCombatWith.delete(characterId);
            if (enemy.targetPlayerId === characterId) {
                enemy.targetPlayerId = null;
                const next = pickNextTarget(enemy);
                if (next) {
                    enemy.targetPlayerId = next;
                } else {
                    enemy.aiState = AI_STATE.IDLE;
                    enemy.isWandering = false;
                }
            }
        }
    }

    // Broadcast player:left to old zone
    broadcastToZone(oldZone, 'player:left', {
        characterId, characterName: player.characterName, reason: 'zone_change'
    });

    // Switch Socket.io rooms
    socket.leave('zone:' + oldZone);
    socket.join('zone:' + newZone);

    // Update player data
    player.zone = newZone;
    player.lastX = warp.destX;
    player.lastY = warp.destY;
    player.lastZ = warp.destZ;

    // Mark as transitioning
    zoneTransitioning.add(characterId);

    // Save to DB
    try {
        await pool.query(
            'UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5',
            [newZone, warp.destX, warp.destY, warp.destZ, characterId]
        );
    } catch (err) {
        logger.warn(`[DB] Failed to save zone change: ${err.message}`);
    }

    // Lazy spawn enemies in new zone
    if (!spawnedZones.has(newZone)) {
        if (newZoneData.enemySpawns.length > 0) {
            for (const spawn of newZoneData.enemySpawns) {
                spawnEnemy({ ...spawn, zone: newZone });
            }
        }
        spawnedZones.add(newZone);
    }

    // Tell client to load new level
    socket.emit('zone:change', {
        zone: newZone,
        displayName: newZoneData.displayName,
        levelName: newZoneData.levelName,
        x: warp.destX, y: warp.destY, z: warp.destZ,
        flags: newZoneData.flags,
        reason: 'warp'
    });
});
```

---

## Appendix A: File Index

| File | Location | Purpose |
|------|----------|---------|
| `ZoneTransitionSubsystem.h` | `client/SabriMMO/Source/SabriMMO/UI/` | Zone transition UWorldSubsystem header |
| `ZoneTransitionSubsystem.cpp` | `client/SabriMMO/Source/SabriMMO/UI/` | Zone transition implementation |
| `WarpPortal.h` | `client/SabriMMO/Source/SabriMMO/` | Warp portal actor header |
| `WarpPortal.cpp` | `client/SabriMMO/Source/SabriMMO/` | Warp portal implementation |
| `KafraNPC.h` | `client/SabriMMO/Source/SabriMMO/` | Kafra NPC actor header |
| `KafraNPC.cpp` | `client/SabriMMO/Source/SabriMMO/` | Kafra NPC implementation |
| `KafraSubsystem.h` | `client/SabriMMO/Source/SabriMMO/UI/` | Kafra service UWorldSubsystem header |
| `KafraSubsystem.cpp` | `client/SabriMMO/Source/SabriMMO/UI/` | Kafra service implementation |
| `SKafraWidget.h` | `client/SabriMMO/Source/SabriMMO/UI/` | Kafra dialog Slate widget header |
| `SKafraWidget.cpp` | `client/SabriMMO/Source/SabriMMO/UI/` | Kafra dialog widget implementation |
| `MMOGameInstance.h` | `client/SabriMMO/Source/SabriMMO/` | GameInstance (zone state persists) |
| `CharacterData.h` | `client/SabriMMO/Source/SabriMMO/` | FCharacterData with ZoneName/LevelName |
| `SabriMMOGameMode.h` | `client/SabriMMO/Source/SabriMMO/` | GameMode base class |
| `ro_zone_data.js` | `server/src/` | Zone registry (all zones, warps, spawns, NPCs) |
| `index.js` | `server/src/` | Server monolith (zone handlers, broadcasting) |
| `add_zone_system.sql` | `database/migrations/` | Zone system DB migration |
| `Zone_System_UE5_Setup_Guide.md` | `docsNew/05_Development/` | Step-by-step UE5 Editor setup guide |

## Appendix B: Socket Event Reference

| Event | Direction | Payload | Handler |
|-------|-----------|---------|---------|
| `zone:warp` | Client -> Server | `{ warpId }` | Validates and triggers zone change |
| `zone:change` | Server -> Client | `{ zone, displayName, levelName, x, y, z, flags, reason }` | Opens new level |
| `zone:ready` | Client -> Server | `{ zone }` | Sends zone enemies/players to client |
| `zone:error` | Server -> Client | `{ message }` | Logs error, no transition |
| `player:teleport` | Server -> Client | `{ characterId, x, y, z, teleportType }` | Teleports pawn in-zone |
| `player:left` | Server -> Client | `{ characterId, characterName, reason }` | Removes player from zone display |
| `kafra:open` | Client -> Server | `{ kafraId }` | Requests Kafra data |
| `kafra:data` | Server -> Client | `{ kafraId, kafraName, currentSaveMap, playerZuzucoin, destinations[] }` | Opens Kafra dialog |
| `kafra:save` | Client -> Server | `{}` | Saves current position as respawn point |
| `kafra:saved` | Server -> Client | `{ saveMap }` | Confirms save point |
| `kafra:teleport` | Client -> Server | `{ kafraId, destZone }` | Requests paid teleport |
| `kafra:teleported` | Server -> Client | `{ destZone, remainingZuzucoin }` | Confirms teleport (zone:change follows) |
| `kafra:error` | Server -> Client | `{ message }` | Error message (insufficient zeny, etc.) |

## Appendix C: Z-Order Reference (Widget Layer Stack)

| Z-Order | Widget | System |
|---------|--------|--------|
| 5 | Login flow widgets | LoginFlowSubsystem |
| 8 | World health bars | WorldHealthBarSubsystem |
| 10 | Basic info (HP/SP/EXP) | BasicInfoSubsystem |
| 12 | Combat stats | CombatStatsSubsystem |
| 14 | Inventory | InventorySubsystem |
| 15 | Equipment | EquipmentSubsystem |
| 16 | Hotbar rows | HotbarSubsystem |
| 19 | Kafra dialog | KafraSubsystem |
| 20 | Skill tree / Damage numbers | SkillTreeSubsystem / DamageNumberSubsystem |
| 25 | Cast bar | CastBarSubsystem |
| 30 | Hotbar keybind config | HotbarSubsystem |
| 50 | Loading overlay (zone + login) | ZoneTransitionSubsystem / LoginFlowSubsystem |

---

**Last Updated**: 2026-03-08
**Version**: 1.0
**Status**: Complete -- Production Reference
