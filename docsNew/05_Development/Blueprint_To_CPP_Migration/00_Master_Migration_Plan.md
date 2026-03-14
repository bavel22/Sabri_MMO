# Blueprint to C++ Migration -- Master Plan

**Status:** NOT STARTED
**Date:** 2026-03-13
**Scope:** Client-side only -- no server changes required
**Systems Touched:** UE5 C++ subsystems, Slate UI, Enhanced Input, Socket.io event routing

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Motivation](#2-motivation)
3. [What Gets Migrated](#3-what-gets-migrated)
4. [What Stays in Blueprint](#4-what-stays-in-blueprint)
5. [New C++ Classes (Architecture)](#5-new-c-classes-architecture)
6. [Phased Approach (6 Phases)](#6-phased-approach-6-phases)
7. [Conflicts and Resolutions](#7-conflicts-and-resolutions)
8. [RO Classic Behavior Reference](#8-ro-classic-behavior-reference)
9. [Server Compatibility](#9-server-compatibility)
10. [File Organization](#10-file-organization)

---

## 1. Project Overview

### Goal

Replace Blueprint-based gameplay systems (movement, targeting, combat, entity management) with pure C++ and Slate UI. The end result is a client where all runtime gameplay logic lives in C++ `UWorldSubsystem` classes and C++ actor classes, with Blueprints remaining only for level setup, animation graphs, and materials.

### Scope Summary

| In Scope | Out of Scope |
|----------|-------------|
| Local player input and movement | Server-side combat/AI logic |
| Camera control | Level Blueprints (zone setup, pawn spawning) |
| Targeting and hover detection | Animation Blueprints (ABP_MMOCharacter) |
| Auto-attack state machine | Material/shader work |
| Remote player management | UMG widgets already replaced by Slate subsystems |
| Enemy management | REST API / auth flow |
| Entity name tags | Existing C++ subsystems (19 already working) |
| Click-to-interact routing | |
| Attack animation triggering | |
| Socket event routing for movement/combat/entity events | |
| NavMesh pathfinding (C++ driven) | |

---

## 2. Motivation

### 2.1 Code Maintainability

Blueprint event graphs for complex logic (e.g., `OnCombatDamage` at 215 nodes, `BP_OtherPlayerManager` at 150+ nodes) are difficult to review, diff, merge, and refactor. C++ provides readable, searchable, diffable source files.

### 2.2 Performance

Blueprint VM overhead on per-tick operations (cursor traces, entity interpolation, position broadcasting) adds up. C++ runs natively with no VM dispatch cost. For systems running every frame on potentially hundreds of entities, this matters.

### 2.3 Type Safety

Blueprint "Get" nodes with unsafe casts (CastTo BP_Enemy, CastTo BP_OtherPlayerCharacter) fail silently at runtime. C++ provides compile-time type checking, interfaces via `UINTERFACE`, and `TWeakObjectPtr` for safe references.

### 2.4 Debugging Ease

Blueprint debugging requires the UE5 editor open with visual breakpoints. C++ debugging uses standard IDE debuggers (Visual Studio, Rider) with conditional breakpoints, watch windows, and call stacks. Log output is grep-able.

### 2.5 Consistency with Existing Architecture

19 C++ subsystems already handle UI, buffs, inventory, equipment, skills, combat stats, damage numbers, zone transitions, and more. The remaining Blueprint systems (movement, targeting, combat actions, entity management) are the last major holdouts. Migrating them completes the architectural pattern.

---

## 3. What Gets Migrated

### 3.1 Local Player Input and Movement

**Source:** BP_MMOCharacter event graph (IA_ClickToMove, IA_Attack, IA_Move bindings)

Click-to-move via NavMesh (`SimpleMoveToLocation`), WASD direct movement override, movement state management. Currently handled by Blueprint event graph nodes on BP_MMOCharacter that receive Enhanced Input actions and route them to movement functions.

### 3.2 Targeting System

**Source:** AC_TargetingSystem (ActorComponent on BP_MMOCharacter)

Per-tick cursor line trace from camera through mouse position, hover detection over enemies/NPCs/players, cursor type changes (attack cursor over enemies, talk cursor over NPCs), target locking, range checking for auto-attack and skill casting.

### 3.3 Camera Control

**Source:** AC_CameraController (ActorComponent on BP_MMOCharacter)

Right-click-drag camera rotation, mouse wheel zoom (distance clamp), fixed pitch angle, camera boom configuration. Currently a Blueprint ActorComponent attached to BP_MMOCharacter.

### 3.4 Auto-Attack State Machine

**Source:** BP_MMOCharacter + BP_SocketManager (OnCombatDamage, OnCombatDeath, etc.)

Click-to-attack initiates walk-to-range, then emits `combat:attack` to server. Server runs ASPD-timed attack loop and sends back `combat:damage` events. Client processes damage results, plays attack animations, handles death overlay and respawn. The `OnCombatDamage` handler alone is 215 Blueprint nodes.

### 3.5 Remote Player Management

**Source:** BP_OtherPlayerManager + BP_OtherPlayerCharacter

Spawning remote player actors when `player:moved` arrives, updating positions with interpolation, removing actors on `player:left`, maintaining a map of active remote players. BP_OtherPlayerManager is a per-level Actor that BP_SocketManager holds a reference to.

### 3.6 Enemy Management

**Source:** BP_EnemyManager + BP_EnemyCharacter

Spawning enemy actors on `enemy:spawn`, updating positions on `enemy:move`, processing `enemy:death` and `enemy:health_update`, death animations and corpse cleanup. BP_EnemyManager is a per-level Actor that BP_SocketManager holds a reference to.

### 3.7 Entity Name Tags

**Source:** WBP_PlayerNameTag (WidgetComponent on BP_OtherPlayerCharacter, BP_EnemyCharacter)

Floating name labels above all entities. Currently uses UMG WidgetComponents attached to each actor. Replacement uses a single Slate overlay rendering all visible name tags via `OnPaint`, which is more efficient for large entity counts.

### 3.8 Click-to-Interact

**Source:** TryInteractWithNPC and related Blueprint logic

NPC detection on click, walk-to-interaction-range, interaction routing (open shop, open dialogue, Kafra services). Currently mixed between AC_TargetingSystem hover detection and BP_MMOCharacter click handling.

### 3.9 Attack Animation Triggering

**Source:** BP_SocketManager.OnCombatDamage

When `combat:damage` arrives, the client faces the target and plays an attack montage. Currently 215 Blueprint nodes handle parsing the damage event, checking if the local player is the attacker, rotating the character, and triggering the animation. This moves into UCombatActionSubsystem.

### 3.10 Socket Event Routing for Movement/Combat/Entity Events

**Source:** BP_SocketManager handler functions

BP_SocketManager currently handles approximately 31 bridged events via MultiplayerEventSubsystem. The movement, combat, and entity events among those will be handled directly by new C++ subsystems registering with `USocketEventRouter`. Remaining events (chat, loot from old path, shop open) stay on BP_SocketManager until separately migrated.

### 3.11 NavMesh Pathfinding

**Stays NavMesh, becomes C++ driven.** The pathfinding infrastructure (NavMesh volumes, navigation data) remains identical. The difference is that `SimpleMoveToLocation` calls originate from C++ (`UPlayerInputSubsystem`) instead of Blueprint event graph nodes.

---

## 4. What Stays in Blueprint

### 4.1 Level Blueprints

Level Blueprints handle zone-specific setup: spawning the player pawn, configuring zone ID, placing warp portal triggers. These are inherently per-level and have no performance or maintainability benefit from C++ migration. See `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md`.

### 4.2 BP_SocketManager (Reduced Role)

BP_SocketManager remains as a handler shell for events not yet migrated to C++ subsystems. As each phase completes, its handler functions become dead code. It is NOT deleted until all events are handled by C++ subsystems. Its SocketIO component is already disconnected (persistent socket lives on GameInstance since Phase 4).

### 4.3 Animation Blueprints

ABP_MMOCharacter (and any enemy animation Blueprints) stay as Blueprint animation graphs. Animation state machines, blend spaces, and montage slots are best authored in the UE5 animation editor. C++ triggers animations by calling `PlayAnimMontage()` or setting variables on the AnimInstance -- it does not replace the animation graph itself.

### 4.4 Materials and Shaders

Material Instances, Material Parameter Collections, and shader graphs stay in Blueprint/editor form. These are visual assets with no gameplay logic.

### 4.5 UMG Widgets Already Replaced

19 C++ Slate subsystems already handle all HUD panels (HP bars, inventory, equipment, skills, buffs, damage numbers, etc.). The old UMG widgets they replaced are already unused. No migration action needed.

---

## 5. New C++ Classes (Architecture)

### 5.1 UWorldSubsystems

All new subsystems follow the established pattern: `UTickableWorldSubsystem` (or `UWorldSubsystem` for non-ticking), register with `USocketEventRouter` in `OnWorldBeginPlay`, unregister in `Deinitialize`. They access `UMMOGameInstance` for socket emit and auth state.

| Subsystem | Replaces | Responsibilities |
|-----------|----------|-----------------|
| `UPlayerInputSubsystem` | BP_MMOCharacter event graph (IA_ClickToMove, IA_Attack, IA_Move logic) | Click-to-move, click-to-attack, click-to-interact input routing, WASD override, movement state machine |
| `UCameraSubsystem` | AC_CameraController | Right-click camera rotation, mouse wheel zoom, pitch/yaw clamping, camera boom distance |
| `UTargetingSubsystem` | AC_TargetingSystem | Per-tick cursor trace, hover detection, cursor type changes (sword/speech/crosshair), target lock, range checks |
| `UOtherPlayerSubsystem` | BP_OtherPlayerManager + BP_SocketManager player handlers | Spawn/update/remove remote players via `TMap<int32, AMMORemotePlayer*>`, socket event registration |
| `UEnemySubsystem` | BP_EnemyManager + BP_SocketManager enemy handlers | Spawn/update/remove enemies via `TMap<int32, AMMOEnemyActor*>`, socket event registration, death state |
| `UCombatActionSubsystem` | BP_SocketManager combat handlers (OnCombatDamage, OnCombatDeath, etc.) | Auto-attack state, `combat:damage` processing, attack animations, facing target, death handling |
| `UNameTagSubsystem` | WBP_PlayerNameTag WidgetComponents | Slate `OnPaint` overlay rendering all entity names (players, enemies, NPCs) with colored text |

### 5.2 Actor Classes

| Class | Replaces | Responsibilities |
|-------|----------|-----------------|
| `AMMORemotePlayer` | BP_OtherPlayerCharacter | Remote player actor: skeletal mesh, capsule collision, `UCharacterMovementComponent` for interpolation, no input |
| `AMMOEnemyActor` | BP_EnemyCharacter | Enemy actor: skeletal mesh, capsule collision, interpolated movement, death state, corpse timer |

### 5.3 Subsystem Interaction Diagram

```
                    USocketEventRouter (on GameInstance)
                             |
          ┌──────────────────┼──────────────────────────┐
          |                  |                          |
  UOtherPlayerSubsystem  UEnemySubsystem  UCombatActionSubsystem
     |                      |                    |
     | Spawns/manages       | Spawns/manages     | Triggers anims on
     v                      v                    v
  AMMORemotePlayer       AMMOEnemyActor     Local pawn (ASabriMMOCharacter)
                                                 ^
                                                 |
                              UPlayerInputSubsystem (click-to-move/attack)
                              UCameraSubsystem (camera control)
                              UTargetingSubsystem (hover/cursor)

  UNameTagSubsystem -- reads positions from all actors, renders Slate overlay
```

### 5.4 Interface Implementation

Both `AMMORemotePlayer` and `AMMOEnemyActor` implement `BPI_Damageable` (via C++ `UINTERFACE`) for:
- `ReceiveDamageVisual(int32 Damage, bool IsCritical)` -- triggers hit flash
- `UpdateHealthDisplay(float CurrentHP, float MaxHP)` -- updates WorldHealthBarSubsystem data

Both implement `BPI_Targetable` for:
- `GetTargetLocation()` -- returns actor center for range checks
- `GetTargetType()` -- returns `ETargetType::Player` or `ETargetType::Enemy`
- `IsTargetAlive()` -- returns death state

---

## 6. Phased Approach (6 Phases)

### Phase 1: Camera + Local Movement (Foundation)

**Goal:** Replace AC_CameraController and BP_MMOCharacter movement logic with C++ subsystems.

**New files:**
- `UI/CameraSubsystem.h` / `UI/CameraSubsystem.cpp`
- `UI/PlayerInputSubsystem.h` / `UI/PlayerInputSubsystem.cpp`

**UCameraSubsystem responsibilities:**
- Right-click-drag camera rotation (yaw only, or yaw + limited pitch)
- Mouse wheel zoom (clamp between min/max boom distance)
- Fixed pitch angle configuration
- Camera boom length interpolation for smooth zoom
- Read mouse input only when right mouse button is held

**UPlayerInputSubsystem responsibilities:**
- Click-to-move: left click on ground triggers `SimpleMoveToLocation` via NavMesh
- WASD movement: direct movement input that cancels click-to-move path
- Movement state machine: `Idle`, `MovingToPoint`, `MovingToTarget`, `MovingWASD`
- Enhanced Input programmatic setup (extend existing pattern from `ASabriMMOCharacter::SetupPlayerInputComponent`)
- Click routing: determine what was clicked (ground, enemy, NPC, player) and route to appropriate action

**Coexistence strategy:**
- During development, both C++ and BP handle input. C++ handler runs first (higher priority Input Mapping Context). BP event graph nodes are disabled one at a time as C++ equivalents are verified.
- No server changes needed -- movement is client-predicted, server validates position.

**Verification:**
- Click on ground: character walks there via NavMesh
- WASD: character moves directly, cancels any click-to-move path
- Right-click drag: camera rotates
- Mouse wheel: camera zooms in/out
- All existing functionality preserved (no regressions in combat, targeting, etc.)

---

### Phase 2: Targeting + Hover Detection

**Goal:** Replace AC_TargetingSystem with UTargetingSubsystem.

**New files:**
- `UI/TargetingSubsystem.h` / `UI/TargetingSubsystem.cpp`

**UTargetingSubsystem responsibilities:**
- Per-tick cursor line trace from camera through screen-space mouse position
- Hover detection: identify what the cursor is over (enemy, NPC, player, ground item, ground)
- Cursor type changes:
  - Default arrow: ground
  - Sword icon: enemy (attackable)
  - Speech bubble: NPC (interactable)
  - Crosshair: skill targeting mode (from SkillTreeSubsystem)
- Target locking: store current target reference (`TWeakObjectPtr<AActor>`)
- Target indicator visuals: highlight ring or selection circle under targeted entity
- Range checking functions: `IsInAttackRange(AActor* Target)`, `IsInInteractionRange(AActor* Target)`, `IsInSkillRange(AActor* Target, float SkillRange)`
- Attack range storage: updated from `inventory:equipped` socket event (weapon range)

**Target priority order:**
1. NPC (highest -- always interactable)
2. Player (PvP check if applicable)
3. Enemy/Monster
4. Ground Item
5. Ground (lowest -- click-to-move)

**Dependencies:**
- Phase 1 must be complete (UPlayerInputSubsystem routes clicks based on UTargetingSubsystem's hover result)
- UPlayerInputSubsystem calls `UTargetingSubsystem::GetHoverTarget()` to decide action on click

**Verification:**
- Hover over enemy: cursor changes to sword
- Hover over NPC: cursor changes to speech bubble
- Hover over ground: cursor is default arrow
- Click enemy: target is locked, selection indicator appears
- Target is cleared when enemy dies or goes out of range

---

### Phase 3: Combat Actions + Auto-Attack

**Goal:** Replace BP_SocketManager combat handlers with UCombatActionSubsystem.

**New files:**
- `UI/CombatActionSubsystem.h` / `UI/CombatActionSubsystem.cpp`

**UCombatActionSubsystem responsibilities:**
- Auto-attack state machine:
  - `Idle` -- no target, no attack
  - `WalkingToTarget` -- moving into attack range
  - `Attacking` -- in range, emitting `combat:attack` to server
  - `TargetDead` -- target died, return to idle
- Socket event registration (via `USocketEventRouter`):
  - `combat:damage` -- process damage result, trigger attack animation, spawn damage number (via DamageNumberSubsystem)
  - `combat:death` -- handle local player death (death overlay, disable input) or remote entity death
  - `combat:stop` -- stop auto-attack
  - `combat:miss` -- play miss animation/effect
- Attack animation triggering:
  - Rotate local player to face target
  - Call `PlayAnimMontage()` on the player's skeletal mesh
  - Animation selection based on weapon type (slash, stab, bow draw, cast)
- Death handling:
  - Local player: show death overlay, disable input, emit `combat:respawn` on button press
  - Remote player/enemy: delegate to UOtherPlayerSubsystem / UEnemySubsystem

**Migration of OnCombatDamage (215 Blueprint nodes):**
The Blueprint handler currently does:
1. Parse JSON payload (damage, targetId, attackerId, isCritical, element, etc.)
2. Check if local player is attacker or target
3. If attacker: face target, play attack animation
4. If target: play hit reaction
5. Spawn damage number via DamageNumberSubsystem
6. Update HP bar via WorldHealthBarSubsystem
7. Handle dual wield second hit (damage2, isCritical2)

All of this maps directly to C++ method calls. JSON parsing uses `TSharedPtr<FJsonObject>` (already used by all existing subsystems).

**Dependencies:**
- Phase 2 must be complete (UTargetingSubsystem provides target reference and range checking)
- DamageNumberSubsystem already exists in C++ (just call its API)
- WorldHealthBarSubsystem already exists in C++ (just call its API)

**Verification:**
- Click enemy: walk to range, auto-attack begins
- Damage numbers appear on hit
- Attack animation plays on local player
- Hit reaction plays on enemy
- Death overlay appears when local player dies
- Respawn works correctly
- Dual wield: both damage numbers appear with Z offset

---

### Phase 4: Entity Management (Other Players + Enemies)

**Goal:** Replace BP_OtherPlayerManager, BP_EnemyManager, BP_OtherPlayerCharacter, and BP_EnemyCharacter with C++ equivalents.

**New files:**
- `UI/OtherPlayerSubsystem.h` / `UI/OtherPlayerSubsystem.cpp`
- `UI/EnemySubsystem.h` / `UI/EnemySubsystem.cpp`
- `MMORemotePlayer.h` / `MMORemotePlayer.cpp`
- `MMOEnemyActor.h` / `MMOEnemyActor.cpp`

**UOtherPlayerSubsystem responsibilities:**
- Entity registry: `TMap<int32, TWeakObjectPtr<AMMORemotePlayer>> RemotePlayers`
- Socket event handlers:
  - `player:moved` -- spawn or update remote player position
  - `player:left` -- remove and destroy remote player actor
  - `player:stats_updated` -- update displayed name, level, class
- Spawn logic: create `AMMORemotePlayer` at received position, configure skeletal mesh, set character name
- Cleanup on zone transition: destroy all remote player actors in `Deinitialize`

**UEnemySubsystem responsibilities:**
- Entity registry: `TMap<int32, TWeakObjectPtr<AMMOEnemyActor>> Enemies`
- Socket event handlers:
  - `enemy:spawn` -- spawn enemy actor at position with template data
  - `enemy:move` -- update enemy position (interpolation target)
  - `enemy:death` -- trigger death animation, start corpse timer, remove from registry
  - `enemy:health_update` -- update HP for WorldHealthBarSubsystem
  - `enemy:respawn` -- respawn enemy at position (reuse or create actor)
- Cleanup on zone transition: destroy all enemy actors in `Deinitialize`

**AMMORemotePlayer (C++ ACharacter subclass):**
- `USkeletalMeshComponent` with character mesh
- `UCapsuleComponent` for collision and targeting traces
- `UCharacterMovementComponent` for smooth interpolation to server positions
- No input component (remote players don't process input)
- Implements `BPI_Damageable` and `BPI_Targetable` via `UINTERFACE`
- Properties: `CharacterName`, `CharacterId`, `JobClass`, `Level`
- Position interpolation: lerp toward target position at configured speed

**AMMOEnemyActor (C++ ACharacter subclass):**
- `USkeletalMeshComponent` with enemy mesh (template-based)
- `UCapsuleComponent` for collision and targeting traces
- `UCharacterMovementComponent` for smooth interpolation
- Implements `BPI_Damageable` and `BPI_Targetable` via `UINTERFACE`
- Properties: `EnemyName`, `EnemyId`, `TemplateId`, `Level`, `CurrentHP`, `MaxHP`, `bIsDead`
- Death state: play death animation, disable collision, start corpse timer, destroy after delay
- Position interpolation: lerp toward target position at configured speed

**Dependencies:**
- Phase 3 must be complete (UCombatActionSubsystem references entities from these subsystems)
- Existing OtherCharacterMovementComponent can be referenced for interpolation logic

**Verification:**
- Enter zone: other players and enemies appear at correct positions
- Other players move smoothly (interpolated)
- Enemies move smoothly (interpolated)
- Player leaves zone: their actor disappears
- Enemy dies: death animation plays, corpse disappears after delay
- Zone transition: all remote entities cleaned up, new zone entities spawn correctly

---

### Phase 5: Name Tags + Interaction

**Goal:** Replace WBP_PlayerNameTag WidgetComponents with a unified Slate overlay, and finalize click-to-interact pipeline.

**New files:**
- `UI/NameTagSubsystem.h` / `UI/NameTagSubsystem.cpp`

**UNameTagSubsystem responsibilities:**
- Slate widget added to viewport (follows existing subsystem Z-order pattern)
- `OnPaint` override: iterate all visible entities, project world position to screen, draw text
- Player name tags: `"CharacterName"` in white text
- Enemy name tags: `"EnemyName Lv.XX"` in white text (or yellow for aggressive)
- NPC name tags: `"NPCName"` in green text
- Visibility culling: only render tags for entities within camera frustum and distance threshold
- No per-entity WidgetComponent overhead -- single draw pass for all tags

**Name tag text colors (RO Classic style):**

| Entity Type | Color | Condition |
|-------------|-------|-----------|
| Local player | White | Always |
| Party member | Green | If party system exists |
| Other player | White | Default |
| Hostile player (PvP) | Red | PvP zone |
| Passive enemy | White | AI code: non-aggressive |
| Aggressive enemy | Yellow | AI code: aggressive |
| Boss enemy | Red | Boss flag |
| NPC | Green | Always |

**Click-to-interact unification in UPlayerInputSubsystem:**
- On left click, UTargetingSubsystem provides the hovered entity and its type
- UPlayerInputSubsystem routes:
  - `ETargetType::Enemy` -> walk to attack range, then auto-attack (via UCombatActionSubsystem)
  - `ETargetType::NPC` -> walk to interaction range, then interact (open shop, dialogue, Kafra)
  - `ETargetType::Player` -> walk to target (future: trade, party invite, inspect)
  - `ETargetType::GroundItem` -> walk to item, then pick up (future: loot system)
  - `ETargetType::Ground` -> click-to-move

**Dependencies:**
- Phase 4 must be complete (AMMORemotePlayer and AMMOEnemyActor provide positions for name tag rendering)
- Existing NPC actors (KafraNPC, WarpPortal) must implement `BPI_Targetable` for type detection

**Verification:**
- All entities show floating name tags
- Name colors match entity type
- Tags scale/fade with distance
- Tags are not visible through walls (optional depth check)
- Click NPC: walk to range, interaction opens
- Click enemy: walk to range, auto-attack starts
- Performance: no frame drop with 50+ visible entities (single OnPaint pass vs. 50 WidgetComponents)

---

### Phase 6: Cleanup + Blueprint Disconnection

**Goal:** Remove all superseded Blueprint components, actors, and event bridges. Clean final state.

**Removals:**

| Remove | Reason |
|--------|--------|
| AC_TargetingSystem component from BP_MMOCharacter | Replaced by UTargetingSubsystem |
| AC_CameraController component from BP_MMOCharacter | Replaced by UCameraSubsystem |
| AC_HUDManager obsolete functions | Already superseded by 19 C++ subsystems |
| BP_OtherPlayerManager actor from all levels | Replaced by UOtherPlayerSubsystem |
| BP_EnemyManager actor from all levels | Replaced by UEnemySubsystem |
| WBP_PlayerNameTag widget | Replaced by UNameTagSubsystem Slate overlay |
| BP_OtherPlayerCharacter class | Replaced by AMMORemotePlayer |
| BP_EnemyCharacter class | Replaced by AMMOEnemyActor |
| BP_MMOCharacter event graph movement/combat nodes | Replaced by UPlayerInputSubsystem + UCombatActionSubsystem |

**MultiplayerEventSubsystem bridge updates:**
- Remove all bridged events now handled by C++ subsystems:
  - `player:moved`, `player:left` (handled by UOtherPlayerSubsystem)
  - `enemy:spawn`, `enemy:move`, `enemy:death`, `enemy:health_update`, `enemy:respawn` (handled by UEnemySubsystem)
  - `combat:damage`, `combat:death`, `combat:stop`, `combat:miss` (handled by UCombatActionSubsystem)
- Remaining bridged events stay on BP_SocketManager until separately migrated

**BP_SocketManager final state:**
- Handler shell for non-migrated events only (chat, loot from old path)
- SocketIO component already disconnected (persistent socket on GameInstance)
- Can be fully removed once all events are handled by C++ subsystems (future work beyond this plan)

**Level Blueprint updates:**
- Remove `SpawnActor BP_OtherPlayerManager` nodes
- Remove `SpawnActor BP_EnemyManager` nodes
- BP_SocketManager spawn may remain if it still handles non-migrated events
- Pawn spawning logic stays (out of scope)

**Verification (full regression):**
- Login flow works (no change expected)
- Zone transition works (no change expected)
- Click-to-move works (C++ driven)
- WASD works (C++ driven)
- Camera rotation and zoom work (C++ driven)
- Hover cursor changes work (C++ driven)
- Auto-attack works (C++ driven)
- Remote players appear and move (C++ driven)
- Enemies appear, move, die, respawn (C++ driven)
- Name tags appear on all entities (Slate OnPaint)
- NPC interaction works (click Kafra, shops, warps)
- All 19 existing C++ subsystems still work (no regressions)
- Damage numbers, HP bars, buff bar, inventory, equipment, skill tree, hotbar, shop, combat stats, cast bar -- all unaffected

---

## 7. Conflicts and Resolutions

### Conflict 1: Enhanced Input Dual Binding

**Issue:** BP_MMOCharacter's event graph binds `IA_Attack`, `IA_ClickToMove`, and `IA_Move` via Enhanced Input. UPlayerInputSubsystem also needs to handle these same input actions. Both cannot process the same input action simultaneously without double-firing.

**Resolution:** Phase 1 adds C++ input handling in `ASabriMMOCharacter` (which BP_MMOCharacter inherits from). The C++ handler uses a higher-priority Input Mapping Context so it runs first. During the transition period, BP event graph nodes are disabled one at a time as their C++ equivalents are verified. In Phase 6, the BP event graph nodes are fully removed. No input action assets need to change -- the same `IA_Attack`, `IA_ClickToMove`, `IA_Move` are reused.

### Conflict 2: BP_SocketManager Event Ownership

**Issue:** `MultiplayerEventSubsystem` bridges approximately 31 socket events to BP_SocketManager handler functions via `ProcessEvent`. New C++ subsystems (UOtherPlayerSubsystem, UEnemySubsystem, UCombatActionSubsystem) need to handle some of these same events directly via `USocketEventRouter`. If both the bridge and the new subsystem handle the same event, logic runs twice.

**Resolution:** New C++ subsystems register directly with `USocketEventRouter` (the same pattern used by all 19 existing C++ subsystems). As each event is migrated to a C++ subsystem, it is removed from `MultiplayerEventSubsystem`'s bridge list. BP_SocketManager handler functions for that event become dead code. This is a gradual, per-event migration -- not a single cutover.

### Conflict 3: Actor Reference Chains

**Issue:** BP_SocketManager stores direct references to `OtherPlayerManagerRef`, `EnemyManagerRef`, and `MMOCharacterRef`. These references are set in Level Blueprint via `Set Variable` nodes. The new C++ subsystems do not use these reference chains.

**Resolution:** New C++ subsystems are independent `UWorldSubsystem` instances -- they do not reference BP managers. Both old (BP) and new (C++) systems can coexist during transition. The old BP managers simply stop receiving events once their corresponding socket events are unregistered from `MultiplayerEventSubsystem`. No reference chain needs to be maintained or updated for the new subsystems.

### Conflict 4: WBP_PlayerNameTag Widget Components

**Issue:** BP_OtherPlayerCharacter and BP_EnemyCharacter each have a `UWidgetComponent` with `WBP_PlayerNameTag` for floating name display. The C++ replacement (AMMORemotePlayer, AMMOEnemyActor) uses `UNameTagSubsystem`'s Slate `OnPaint` overlay instead. During the transition (Phase 4 complete, Phase 5 not yet done), entities could have either WidgetComponent tags or Slate tags, but not both.

**Resolution:** Phase 4 creates `AMMORemotePlayer` and `AMMOEnemyActor` WITHOUT WidgetComponent name tags. During Phase 4, entity names are temporarily invisible (or rendered by a minimal interim solution). Phase 5 adds `UNameTagSubsystem` which renders all names via Slate overlay, completing the swap. This is acceptable because Phase 4 and Phase 5 are developed sequentially and can be shipped together.

### Conflict 5: BPI_Damageable Interface

**Issue:** BP_OtherPlayerCharacter and BP_EnemyCharacter implement `BPI_Damageable` as a Blueprint Interface. C++ code calling interface functions on Blueprint actors uses `Execute_` static functions. The new C++ actors need to implement the same interface so that existing systems (WorldHealthBarSubsystem, DamageNumberSubsystem) can interact with them.

**Resolution:** `AMMORemotePlayer` and `AMMOEnemyActor` implement `BPI_Damageable` and `BPI_Targetable` via C++ `UINTERFACE`. The interface function signatures remain identical. `UCombatActionSubsystem` calls interface functions on C++ actors directly via `Cast<IBPIDamageable>(Actor)->ReceiveDamageVisual()`. WorldHealthBarSubsystem already handles HP bars independently of the interface.

### Conflict 6: Attack Range from Server

**Issue:** When the player equips a weapon, `OnItemEquipped` (in BP_SocketManager) sets `AttackRange` on both `AC_TargetingSystem` and `BP_MMOCharacter`. The new `UTargetingSubsystem` needs this value to determine when the player is in attack range.

**Resolution:** `UTargetingSubsystem` stores `AttackRange` as a member variable. It is updated when the `inventory:equipped` socket event arrives. `EquipmentSubsystem` already processes this event in C++ -- it can notify `UTargetingSubsystem` directly, or `UTargetingSubsystem` can register for the same event independently via `USocketEventRouter`. The value defaults to melee range (1 cell) if no weapon is equipped.

---

## 8. RO Classic Behavior Reference

This section documents the expected player-facing behavior that the C++ migration must preserve exactly. All behavior is defined by RO Classic (pre-renewal).

### Movement

- **Click-to-move:** Left click on ground pathfinds via NavMesh and walks the character there. Subsequent clicks cancel the current path and start a new one.
- **WASD:** Direct movement override. Pressing any WASD key cancels the current click-to-move path and switches to direct movement. Releasing all keys returns to idle.
- **Click-to-attack:** Left click on an enemy initiates walk-to-attack-range. Once in range, the client emits `combat:attack` and the server begins the auto-attack loop.
- **Click-to-interact:** Left click on an NPC initiates walk-to-interaction-range. Once in range, the interaction opens (shop, dialogue, Kafra).

### Cursor States

| Cursor | Condition |
|--------|-----------|
| Default arrow | Hovering over ground or no target |
| Sword | Hovering over attackable enemy |
| Speech bubble | Hovering over interactable NPC |
| Crosshair | Skill targeting mode active (ground AoE or targeted skill) |

### Target Priority

When multiple entities overlap under the cursor, selection follows this priority:

1. **NPC** -- highest priority (always interactable)
2. **Player** -- for future PvP, trade, party invite
3. **Monster/Enemy** -- attackable
4. **Ground Item** -- lootable (future)
5. **Ground** -- click-to-move (lowest)

### Auto-Attack

- Server-authoritative: client emits `combat:attack`, server runs ASPD-timed loop
- Client shows attack animation and damage numbers from `combat:damage` events
- Auto-attack continues until: target dies, player moves, player clicks elsewhere, or player presses stop key
- ASPD determines attack speed (frames between attacks), capped at 190 (or 195 for single wield)

### Name Tags

- Floating text above entity's head, always facing camera (billboard)
- Player: `"CharacterName"` in white
- Enemy: `"EnemyName Lv.XX"` in white (passive) or yellow (aggressive)
- NPC: `"NPCName"` in green
- Boss: `"BossName Lv.XX"` in red

---

## 9. Server Compatibility

**This migration is entirely client-side. No server changes are required.**

The C++ subsystems register as listeners via `USocketEventRouter` and process the exact same JSON payloads that BP_SocketManager currently handles. The socket events emitted by the client (`combat:attack`, `combat:stop`, `player:position`, etc.) remain identical in format and timing.

### Events consumed by new subsystems (same payloads as today):

| Event | Consumer | Payload (unchanged) |
|-------|----------|---------------------|
| `player:moved` | UOtherPlayerSubsystem | `{ id, x, y, z, name, jobClass, ... }` |
| `player:left` | UOtherPlayerSubsystem | `{ id }` |
| `enemy:spawn` | UEnemySubsystem | `{ id, templateId, x, y, z, name, level, hp, maxHp, ... }` |
| `enemy:move` | UEnemySubsystem | `{ id, x, y, z }` |
| `enemy:death` | UEnemySubsystem | `{ id, killerId }` |
| `enemy:health_update` | UEnemySubsystem | `{ id, hp, maxHp }` |
| `combat:damage` | UCombatActionSubsystem | `{ attackerId, targetId, damage, isCritical, element, ... }` |
| `combat:death` | UCombatActionSubsystem | `{ targetId, killerId }` |
| `combat:stop` | UCombatActionSubsystem | `{ }` |

### Events emitted by new subsystems (same payloads as today):

| Event | Emitter | Payload (unchanged) |
|-------|---------|---------------------|
| `combat:attack` | UCombatActionSubsystem | `{ targetId, isEnemy }` |
| `combat:stop` | UCombatActionSubsystem | `{ }` |
| `player:position` | PositionBroadcastSubsystem (already C++) | `{ x, y, z }` |

---

## 10. File Organization

All new files live in `client/SabriMMO/Source/SabriMMO/`, following existing conventions.

### Subsystems (in `UI/` directory)

Following the established pattern where all `UWorldSubsystem` classes live in the `UI/` subdirectory (even non-UI subsystems like `PositionBroadcastSubsystem` and `MultiplayerEventSubsystem`):

```
UI/CameraSubsystem.h
UI/CameraSubsystem.cpp
UI/PlayerInputSubsystem.h
UI/PlayerInputSubsystem.cpp
UI/TargetingSubsystem.h
UI/TargetingSubsystem.cpp
UI/OtherPlayerSubsystem.h
UI/OtherPlayerSubsystem.cpp
UI/EnemySubsystem.h
UI/EnemySubsystem.cpp
UI/CombatActionSubsystem.h
UI/CombatActionSubsystem.cpp
UI/NameTagSubsystem.h
UI/NameTagSubsystem.cpp
```

### Actor Classes (in root source directory)

Following the pattern of `SabriMMOCharacter.*` and `OtherCharacterMovementComponent.*`:

```
MMORemotePlayer.h
MMORemotePlayer.cpp
MMOEnemyActor.h
MMOEnemyActor.cpp
```

### Total New Files

- 14 new `.h` files (7 subsystems + 2 actors = 9, but some subsystems are header-only patterns... all have both)
- 14 new `.cpp` files
- **18 new files total** (7 subsystem pairs + 2 actor pairs = 18)

### Module Registration

All new classes must be added to the `SabriMMO.Build.cs` module if additional dependencies are required (e.g., `NavigationSystem` for NavMesh pathfinding). Current module dependencies already include `SocketIOClient`, `JsonUtilities`, `SlateCore`, `InputCore`, and `EnhancedInput`.

---

## Progress Tracker

| Phase | Status | Files Created | Blueprint Removed |
|-------|--------|---------------|-------------------|
| **Phase 1: Camera + Movement** | NOT STARTED | CameraSubsystem, PlayerInputSubsystem | (coexist) |
| **Phase 2: Targeting** | NOT STARTED | TargetingSubsystem | (coexist) |
| **Phase 3: Combat Actions** | NOT STARTED | CombatActionSubsystem | (coexist) |
| **Phase 4: Entity Management** | NOT STARTED | OtherPlayerSubsystem, EnemySubsystem, MMORemotePlayer, MMOEnemyActor | (coexist) |
| **Phase 5: Name Tags + Interaction** | NOT STARTED | NameTagSubsystem | (coexist) |
| **Phase 6: Cleanup** | NOT STARTED | (none) | AC_TargetingSystem, AC_CameraController, BP_OtherPlayerManager, BP_EnemyManager, WBP_PlayerNameTag, BP event graph nodes |
