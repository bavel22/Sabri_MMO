# 00 -- Master Implementation Plan

## Ragnarok Online Classic 3D Replica -- Sabri_MMO

**Document Version**: 1.0
**Date**: 2026-03-08
**Engine**: Unreal Engine 5.7 (C++ + Blueprints via Claude Code)
**Server**: Node.js 18+ / Express / Socket.io 4.8
**Database**: PostgreSQL 15.4 + Redis 7.2
**Architecture**: Server-authoritative, pure C++ client, Slate UI, UWorldSubsystem pattern

---

## Table of Contents

1. [UE5 C++ Architecture Principles](#1-ue5-c-architecture-principles)
2. [What Is Already Built](#2-what-is-already-built)
3. [Build Order -- Critical Path](#3-build-order----critical-path)
4. [Task Checklist -- All Phases](#4-task-checklist----all-phases)
5. [Per-Phase Milestone Definitions](#5-per-phase-milestone-definitions)
6. [Implementation Guide Index](#6-implementation-guide-index)
7. [Cross-Cutting Rules](#7-cross-cutting-rules)

---

## 1. UE5 C++ Architecture Principles

These principles govern every line of code written in this project. They are non-negotiable.

### 1.1 Pure C++ -- No Manual Blueprint Work

All gameplay logic, UI, and systems are written in C++. Claude Code can create and modify Blueprints via `unrealMCP`, but the developer never opens the Blueprint editor to hand-wire nodes. This ensures:
- All code is version-controlled in git
- No binary-only logic that cannot be diffed or reviewed
- Reproducible builds from source alone (plus referenced assets)

**Exceptions**: Level Blueprints (spawning pawn, position save timer) are duplicated from a template level. UMG widgets exist from earlier development but new UI is Slate only.

### 1.2 UWorldSubsystem for All Game Systems

Every discrete game system gets its own `UWorldSubsystem`. This pattern provides:
- **Automatic lifecycle**: created when a world loads, destroyed when it unloads
- **No singleton globals**: each PIE instance gets its own subsystem (multiplayer-safe)
- **Clean separation**: subsystems do not reference each other directly; they communicate via delegates or through the GameInstance
- **Discoverable**: `GetWorld()->GetSubsystem<UMySubsystem>()` from any world-context object

Subsystems do NOT support replication. All authoritative state lives on the Node.js server. Subsystems are presentation + input bridges between socket events and Slate widgets.

**Subsystem selection guide**:
| Scope | Class | Use When |
|-------|-------|----------|
| Entire game session | `UGameInstanceSubsystem` | Auth tokens, server URL, character data that persists across level loads |
| Per-world/level | `UWorldSubsystem` | All gameplay systems: combat, inventory, party, VFX, UI panels |
| Per-local-player | `ULocalPlayerSubsystem` | Player-specific settings, keybinds, camera preferences |

### 1.3 Slate Widgets for All UI

All new UI panels use Slate (the C++ UI framework underlying UMG). Benefits:
- No `.uasset` binary files -- pure `.h`/`.cpp`
- Full C++ control over layout, rendering, and input
- Compile-time type checking on bindings
- Better performance than UMG for programmatic UI

**Widget naming**: `S` prefix (e.g., `SInventoryWidget`, `SPartyWidget`).
**Z-order registry**: Every widget has a fixed Z-order to prevent overlap conflicts (see `docsNew/09_UI_UX_System.md`).
**Viewport access**: NEVER use `GEngine->GameViewport`. Always use `World->GetGameViewport()` from the owning subsystem.

### 1.4 GameInstance for Cross-Level Persistence

`UMMOGameInstance` holds:
- Auth token (JWT)
- Selected character data (`FCharacterData`)
- Server URL
- Zone transition state (`CurrentZoneName`, `PendingZoneName`, `PendingSpawnLocation`, `bIsZoneTransitioning`)
- Remembered username

**Rule**: If data must survive a level transition (OpenLevel), it goes in GameInstance. If data is level-specific, it goes in a UWorldSubsystem.

### 1.5 Manager Pattern -- One Manager Per Domain

Each domain of managed objects (other players, enemies, NPCs, pets) gets one manager:
- Holds a `TMap<FString, AActor*>` or similar container
- Provides `Register()`, `Unregister()`, `Get()`, `GetAll()` functions
- Handles spawn/despawn lifecycle
- Listens to relevant socket events

Managers can be Actors (if they need world presence, like BP_OtherPlayerManager) or UWorldSubsystems (preferred for new systems).

### 1.6 Interfaces Over Cast Chains

Use UE5 C++ interfaces (`IInterface`) to decouple systems:
- `BPI_Damageable` -- anything that can take damage (players, enemies, destructibles)
- `BPI_Interactable` -- anything the player can click on (NPCs, chests, portals)
- `BPI_Targetable` -- anything that can be targeted by skills

**Rule**: Never `Cast<ABP_Enemy>(Actor)`. Instead: `if (Actor->Implements<UDamageableInterface>())`.

### 1.7 Event Dispatchers Over Tick Polling

Widgets bind to delegates (`OnHealthChanged`, `OnInventoryUpdated`, `OnPartyMemberJoined`). They do NOT poll state on Tick.

**Pattern**:
```
Server event arrives via Socket.io
  -> Subsystem processes event, updates local state
  -> Subsystem broadcasts delegate
  -> Widget(s) receive delegate, update display
```

### 1.8 Component-Based Architecture

New gameplay features get a dedicated `UActorComponent`:
- `UCombatComponent` for combat state
- `UMountComponent` for Peco Peco state
- `UPetFollowerComponent` for pet AI

Do NOT add movement + combat + inventory + social logic to one Actor class.

### 1.9 Server-Authoritative Everything

The client NEVER computes authoritative game state. The client:
- Sends INPUT to the server (movement requests, skill use requests, trade requests)
- Receives RESULTS from the server (damage dealt, item obtained, position corrected)
- DISPLAYS results to the player

The server:
- Validates all inputs
- Computes all outcomes (damage, drops, EXP, stat changes)
- Persists all state (PostgreSQL for durable, Redis for ephemeral)
- Broadcasts results to affected clients

---

## 2. What Is Already Built

Status as of 2026-03-08. Items marked DONE require no further work for their core functionality. Items marked PARTIAL have working foundations but need expansion.

### 2.1 Fully Complete Systems (DONE)

| System | Key Files | Socket Events | Notes |
|--------|-----------|---------------|-------|
| **JWT Auth + Login** | `MMOGameInstance.*`, `MMOHttpManager.*`, `LoginFlowSubsystem.*` | REST `/api/auth/*` | Login, register, remember username, configurable server URL |
| **Character CRUD** | `CharacterData.h`, `SCharacterSelectWidget.*`, `SCharacterCreateWidget.*` | REST `/api/characters/*` | 9 slots, customization (hair 1-19, color 0-8, gender), soft-delete |
| **Server Selection** | `SServerSelectWidget.*` | REST `/api/servers` | Population, status display |
| **Login Flow State Machine** | `LoginFlowSubsystem.*`, 5 Slate widgets | -- | Login -> Server -> CharSelect -> Create -> EnterWorld |
| **Basic Multiplayer** | `SabriMMOCharacter.*`, `OtherCharacterMovementComponent.*` | `player:join/position/moved/left` | Position sync, remote player interpolation, name tags |
| **Stat System** | `ro_damage_formulas.js` | `player:stats`, `stats:allocate` | 6 base stats, derived stats, allocation with RO formula |
| **EXP + Leveling** | `ro_exp_tables.js` | implicit in `combat:damage` | Base EXP 1-99, Job EXP tables (Novice/1st/2nd), class config |
| **Inventory** | `InventorySubsystem.*`, `SInventoryWidget.*` | `inventory:data/use/equip/unequip/drop/move` | 148 items, drag-drop, stackable, equipped weapon modifies stats |
| **Equipment** | `EquipmentSubsystem.*`, `SEquipmentWidget.*` | wraps `inventory:data` | All slots, dual accessories, weapon stat integration |
| **Hotbar** | `HotbarSubsystem.*`, `SHotbarRowWidget.*` | `hotbar:save/save_skill/clear/alldata/request` | 4 rows x 9 slots, keybinds, server persistence |
| **Basic Info HUD** | `BasicInfoSubsystem.*`, `SBasicInfoWidget.*` | `player:stats` | HP/SP/EXP bars, draggable panel |
| **Combat Stats Panel** | `CombatStatsSubsystem.*`, `SCombatStatsWidget.*` | `player:stats` | F8 toggle, full stat display |
| **Damage Numbers** | `DamageNumberSubsystem.*`, `SDamageNumberOverlay.*` | `combat:damage`, `skill:effect_damage` | World-to-screen projected floating numbers |
| **World Health Bars** | `WorldHealthBarSubsystem.*`, `SWorldHealthBarOverlay.*` | `player:stats`, `enemy:spawned` | Floating HP/SP bars, RO Classic colors |
| **Cast Bars** | `CastBarSubsystem.*`, `SCastBarOverlay.*` | `skill:cast_start/complete/interrupted` | World-projected, DEX reduction |
| **Skill Tree UI** | `SkillTreeSubsystem.*`, `SSkillTreeWidget.*` | `skill:data/learn/reset` | K toggle, prerequisite validation |
| **Zone Transitions** | `ZoneTransitionSubsystem.*`, `WarpPortal.*` | `zone:change/error`, `player:teleport` | Loading overlay, pawn teleport, zone flags |
| **Kafra NPC** | `KafraSubsystem.*`, `SKafraWidget.*`, `KafraNPC.*` | `kafra:data/saved/teleported/error` | Save point, teleport service |
| **Shop System** | `ShopSubsystem.*`, `SShopWidget.*`, `ShopNPC.*` | `shop:open/buy/sell/buy_batch/sell_batch` | Batch operations, Discount/Overcharge integration |
| **Job Change** | server `job:change` handler | `job:change` | Novice->1st (JLv10), 1st->2nd (JLv40+), system announcement |
| **Damage Formulas** | `ro_damage_formulas.js` | -- | Physical, magical, element 10x10x4, size penalty, HIT/FLEE, crit |
| **Loading Overlay** | `SLoadingOverlayWidget.*` | -- | Fully opaque, progress bar |

### 2.2 Partially Complete Systems (PARTIAL)

| System | What Works | What Is Missing |
|--------|-----------|-----------------|
| **Skill Handlers** | 15 skills implemented (Heal, First Aid, Provoke, 3 Bolts, Fire Ball, Soul Strike, Thunderstorm, Frost Diver, Stone Curse, Sight, Napalm Beat, Fire Wall, Safety Wall) | ~71 remaining skills (Bash, Magnum Break, Endure, Double Strafe, Arrow Shower, Blessing, Increase AGI, all 2nd class skills) |
| **VFX System** | 5 patterns (Bolt, AoE Projectile, Multi-Hit, Persistent Buff, Ground Rain), ~10 skills have VFX | VFX for ~76 remaining skills |
| **Status Effects** | Freeze (Frost Diver), Petrify (Stone Curse), Provoke ATK/DEF mod | Poison, Stun, Sleep, Curse, Silence, Blind, Bleeding, Coma |
| **Enemy AI** | 509 templates, 46 spawns, full state machine, 18 mode flags, aggro/assist | Monster skills (none), MVP bosses (none), mini-bosses (none) |
| **Zone/Map System** | 4 zones (Prontera town, south field, north field, dungeon 1) | 26+ additional zones planned |
| **Chat** | Global chat working | Zone, Party, Guild, Whisper, Trade channels missing |
| **ASPD Formula** | Generic sqrt-based | Need per-class per-weapon tables |
| **Weight System** | Weight column on items | Not enforced (no overweight penalties) |
| **Card System** | 23 cards in DB with descriptions | No compound logic, no effect application |
| **Combat** | Auto-attack loop, 50ms tick, ASPD-based | Size/race/element not always passed; critical needs refinement |

### 2.3 Not Started Systems

| System | Phase | Companion Doc |
|--------|-------|---------------|
| Party system | Phase 6 | `08_PvP_Guild_WoE.md` |
| Guild system | Phase 6 | `08_PvP_Guild_WoE.md` |
| PvP + War of Emperium | Phase 7 | `08_PvP_Guild_WoE.md` |
| Quest system | Phase 8 | `07_NPCs_Quests_Shops.md` |
| NPC dialogue engine | Phase 8 | `07_NPCs_Quests_Shops.md` |
| Trading (player-to-player) | Phase 9 | `13_Economy_Trading_Vending.md` |
| Vending (player shops) | Phase 9 | `13_Economy_Trading_Vending.md` |
| Buying Store | Phase 9 | `13_Economy_Trading_Vending.md` |
| Kafra Storage | Phase 9 | `13_Economy_Trading_Vending.md` |
| Pet system | Phase 10 | `12_Pets_Homunculus_Companions.md` |
| Homunculus | Phase 10 | `12_Pets_Homunculus_Companions.md` |
| Falcon / Peco Peco / Cart | Phase 10 | `12_Pets_Homunculus_Companions.md` |
| Equipment refining | Phase 5 | `05_Items_Equipment_Cards.md` |
| Minimap + World map | Phase 11 | `09_UI_UX_System.md` |
| Character models | Phase 11 | `10_Art_Animation_VFX_Pipeline.md` |
| Monster models | Phase 11 | `10_Art_Animation_VFX_Pipeline.md` |
| Audio (BGM + SFX) | Phase 12 | `14_Audio_Music_SFX.md` |
| Transcendent classes | Post-launch | `01_Stats_Leveling_JobSystem.md` |

---

## 3. Build Order -- Critical Path

Systems MUST be built in this order. Each phase depends on all phases above it. Within a phase, tasks are ordered by internal dependency.

```
PHASE 0: Foundation Hardening
    |
    +-- PHASE 1: Core Combat Completion
    |       |
    |       +-- PHASE 5: Items & Equipment Deep Dive
    |       |       |
    |       |       +-- (feeds into Phase 9: Economy)
    |       |
    |       +-- PHASE 2: Class System
    |       |       |
    |       |       +-- PHASE 10: Companions (needs classes + items)
    |       |       |
    |       |       +-- (feeds into Phase 7: PvP/WoE)
    |       |
    |       +-- PHASE 4: Monster System Expansion (needs zones from P3)
    |
    +-- PHASE 3: World Expansion (can run in parallel with P1 after P0)
    |       |
    |       +-- PHASE 8: NPC & Quest System
    |
    +-- PHASE 6: Social Systems (needs combat from P1)
    |       |
    |       +-- PHASE 7: PvP & War of Emperium (needs guilds from P6, classes from P2)
    |
    PHASE 11: Art & Polish  -------- runs alongside all phases
    PHASE 12: Audio         -------- runs alongside all phases
    PHASE 13: Optimization  -------- runs after all gameplay systems
```

### Recommended Execution Sequence (Solo Developer)

| Order | Phase | Duration | Parallel Work |
|-------|-------|----------|---------------|
| 1 | Phase 0: Foundation Hardening | 2-3 weeks | -- |
| 2 | Phase 1: Core Combat Completion | 4-6 weeks | -- |
| 3 | Phase 5: Items & Equipment | 4 weeks | Natural overlap with combat testing |
| 4 | Phase 2: Class System | 6-8 weeks | -- |
| 5 | Phase 3: World Expansion | 8+ weeks | Begin alongside Phase 4 |
| 6 | Phase 4: Monster Expansion | 4 weeks | Concurrent with Phase 3 |
| 7 | Phase 6: Social Systems | 4-6 weeks | -- |
| 8 | Phase 8: NPC & Quest System | 3-5 weeks | -- |
| 9 | Phase 9: Economy | 3-4 weeks | -- |
| 10 | Phase 10: Companion Systems | 4-6 weeks | -- |
| 11 | Phase 7: PvP & War of Emperium | 4-6 weeks | -- |
| 12 | Phase 11: Art & Polish | 8-12 weeks (focused) | Ongoing throughout |
| 13 | Phase 12: Audio | 3-4 weeks | -- |
| 14 | Phase 13: Optimization & Launch | 4-6 weeks | -- |

**Total**: 60-90 weeks (15-22 months) solo developer full-time.

---

## 4. Task Checklist -- All Phases

### Legend

- `[x]` = DONE (already implemented and working)
- `[ ]` = TODO
- Complexity: **S** = hours, **M** = 1-3 days, **L** = 1-2 weeks, **XL** = 2+ weeks
- Dependencies listed as `Dep: <task reference>`

---

### PHASE 0: Foundation Hardening

**Goal**: Stabilize codebase, resolve tech debt, establish performance baselines.
**Guide**: `00_Master_Build_Plan.md` Section 2, `11_Multiplayer_Networking.md`
**Dependencies**: None (enables all other phases)

#### 0.1 Server Modularization

- [ ] **0.1.1** Extract shared utilities (findPlayer, broadcastToZone, broadcastToZoneExcept, logger, constants) into `server/src/shared/` -- **M**
  - [ ] `server/src/shared/db.js` -- PostgreSQL pool + Redis client
  - [ ] `server/src/shared/logger.js` -- Structured logging
  - [ ] `server/src/shared/constants.js` -- All game constants
  - [ ] `server/src/shared/utils.js` -- Player lookup, zone broadcast, distance calc
- [ ] **0.1.2** Extract auth module: `server/src/modules/auth.js` -- REST endpoints + JWT middleware -- **M**. Dep: 0.1.1
- [ ] **0.1.3** Extract character module: `server/src/modules/characters.js` -- Character CRUD REST -- **M**. Dep: 0.1.1
- [ ] **0.1.4** Extract player module: `server/src/modules/players.js` -- join/position/disconnect handlers -- **M**. Dep: 0.1.1
- [ ] **0.1.5** Extract combat module: `server/src/modules/combat.js` -- Auto-attack loop, damage wrappers -- **L**. Dep: 0.1.1
- [ ] **0.1.6** Extract skills module: `server/src/modules/skills.js` -- skill:use mega-handler, cast system -- **L**. Dep: 0.1.5
- [ ] **0.1.7** Extract inventory module: `server/src/modules/inventory.js` -- All inventory/equip events -- **M**. Dep: 0.1.1
- [ ] **0.1.8** Extract enemies module: `server/src/modules/enemies.js` -- AI loop, spawn management -- **L**. Dep: 0.1.5
- [ ] **0.1.9** Extract zones module: `server/src/modules/zones.js` -- Zone transitions, warp events -- **M**. Dep: 0.1.1
- [ ] **0.1.10** Extract chat module: `server/src/modules/chat.js` -- Message routing -- **S**. Dep: 0.1.1
- [ ] **0.1.11** Extract shops module: `server/src/modules/shops.js` -- NPC shop events -- **M**. Dep: 0.1.1
- [ ] **0.1.12** Extract stats module: `server/src/modules/stats.js` -- Stat allocation, derived calc -- **M**. Dep: 0.1.1
- [ ] **0.1.13** Extract buffs module: `server/src/modules/buffs.js` -- Buff/debuff tracking, expiry -- **M**. Dep: 0.1.5
- [ ] **0.1.14** Wire all modules in `server/src/index.js` (~300 line orchestrator) -- **M**. Dep: 0.1.2 through 0.1.13
- [ ] **0.1.15** Regression test: all existing socket events still work after modularization -- **M**. Dep: 0.1.14

#### 0.2 Skill Handler Refactoring

- [ ] **0.2.1** Extract common skill patterns into reusable functions -- **L**. Dep: 0.1.6
  - [ ] `boltPattern(skillConfig)` -- Cold/Fire/Lightning Bolt
  - [ ] `splashPattern(skillConfig)` -- Napalm Beat, Fire Ball
  - [ ] `multiHitPattern(skillConfig)` -- Soul Strike
  - [ ] `groundAoePattern(skillConfig)` -- Thunderstorm
  - [ ] `debuffPattern(skillConfig)` -- Frost Diver, Stone Curse
  - [ ] `selfBuffPattern(skillConfig)` -- Sight, Endure
  - [ ] `healPattern(skillConfig)` -- Heal, First Aid
  - [ ] `physicalSinglePattern(skillConfig)` -- Bash, Mammonite
  - [ ] `physicalAoePattern(skillConfig)` -- Magnum Break, Arrow Shower
- [ ] **0.2.2** Migrate existing 15 skill handlers to use pattern functions -- **M**. Dep: 0.2.1
- [ ] **0.2.3** Verify all 15 skills still function identically after refactor -- **M**. Dep: 0.2.2

#### 0.3 Performance Baseline

- [ ] **0.3.1** Create load test script (`scripts/load_test.js`) simulating 50+ concurrent connections -- **M**
- [ ] **0.3.2** Measure and document baseline metrics -- **S**. Dep: 0.3.1
  - REST response time (p50, p95, p99)
  - Socket.io event round-trip latency
  - Combat tick accuracy (drift from 50ms target)
  - Client FPS with 20 players + 50 enemies
  - Server memory usage
  - DB query times (p50, p95)
- [ ] **0.3.3** Save baseline document as `docsNew/05_Development/Performance_Baseline.md` -- **S**. Dep: 0.3.2

#### 0.4 Database Optimization

- [ ] **0.4.1** Add composite indexes -- **S**
  - `character_inventory(character_id, is_equipped)`
  - `character_hotbar(character_id, row_index)`
  - `characters(user_id, deleted)`
- [ ] **0.4.2** Add DB connection retry logic on startup failure -- **S**
- [ ] **0.4.3** Audit and fix N+1 queries in skill:data, inventory:load handlers -- **M**. Dep: 0.1.6, 0.1.7
- [ ] **0.4.4** Create migration: `database/migrations/add_performance_indexes.sql` -- **S**

#### 0.5 Security and Code Cleanup

- [ ] **0.5.1** Add position validation to `player:position` handler (speed check, max delta per tick) -- **M**. Dep: 0.1.4
- [ ] **0.5.2** Standardize error handling across ALL socket handlers (consistent `event:error` format) -- **M**. Dep: 0.1.14
- [ ] **0.5.3** Review and add `IsValid()` null guards in all C++ subsystems -- **M**
- [ ] **0.5.4** Remove or archive `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` directories -- **S**
- [ ] **0.5.5** Create socket event reference document listing all events, directions, and payloads -- **M**. Dep: 0.1.14

---

### PHASE 1: Core Combat Completion

**Goal**: Complete RO pre-renewal combat -- all first-class skill handlers, full status effect system, accurate ASPD, element/size/race integration.
**Guide**: `02_Combat_System.md`, `03_Skills_Complete.md`, `01_Stats_Leveling_JobSystem.md`
**Dependencies**: Phase 0 (modularized server)

#### 1.1 ASPD Formula Accuracy

- [ ] **1.1.1** Create `server/src/ro_aspd_tables.js` with per-class per-weapon-type base ASPD values -- **M**
- [ ] **1.1.2** Replace generic sqrt ASPD formula in `ro_damage_formulas.js` with table lookup -- **M**. Dep: 1.1.1
- [ ] **1.1.3** Wire equipped weapon type into ASPD calculation -- **S**. Dep: 1.1.2
- [ ] **1.1.4** Test ASPD values match RO classic for all 6 first classes -- **S**. Dep: 1.1.3

#### 1.2 Status Effect System

- [ ] **1.2.1** Design status effect registry in `server/src/modules/buffs.js` -- **M**. Dep: Phase 0
  - `applyStatusEffect(target, statusId, duration, sourceId)`
  - `removeStatusEffect(target, statusId)`
  - `tickStatusEffects(target)` -- called from combat tick
  - `hasStatusEffect(target, statusId)`
  - `getActiveEffects(target)`
- [ ] **1.2.2** Implement Poison status (ID 1) -- -HP/3s tick, -50% HP regen -- **M**. Dep: 1.2.1
- [ ] **1.2.3** Implement Stun status (ID 2) -- cannot move/attack/cast, 2-5s -- **M**. Dep: 1.2.1
- [ ] **1.2.4** Verify Freeze status (ID 3) -- already done via Frost Diver, wire into generic framework -- **S**. Dep: 1.2.1
- [ ] **1.2.5** Verify Petrify status (ID 4) -- already done via Stone Curse, wire into generic framework -- **S**. Dep: 1.2.1
- [ ] **1.2.6** Implement Sleep status (ID 5) -- cannot act, wake on damage -- **M**. Dep: 1.2.1
- [ ] **1.2.7** Implement Curse status (ID 6) -- LUK=0, -25% move speed, 30s -- **M**. Dep: 1.2.1
- [ ] **1.2.8** Implement Silence status (ID 7) -- cannot use skills, 30s -- **M**. Dep: 1.2.1
- [ ] **1.2.9** Implement Blind status (ID 8) -- -25 HIT, -25% FLEE, 30s -- **M**. Dep: 1.2.1
- [ ] **1.2.10** Implement Bleeding status (ID 9) -- -HP/5s tick, no regen, 30s -- **M**. Dep: 1.2.1
- [ ] **1.2.11** Implement Coma status (ID 10) -- HP=1, SP=1 instant -- **S**. Dep: 1.2.1
- [ ] **1.2.12** Add `status:applied` / `status:removed` / `status:tick` socket events -- **M**. Dep: 1.2.1
- [ ] **1.2.13** Create `StatusEffectSubsystem` (C++ UWorldSubsystem) for client-side status tracking and icon display -- **L**. Dep: 1.2.12
- [ ] **1.2.14** Create `SStatusEffectWidget` (Slate) showing active status icons with remaining duration -- **M**. Dep: 1.2.13
- [ ] **1.2.15** Add status effect VFX patterns (Poison green drip, Stun stars, Sleep Zzz, etc.) -- **L**. Dep: 1.2.13

#### 1.3 Swordsman Skill Handlers (5 remaining)

- [x] Provoke (104) -- DONE
- [ ] **1.3.1** Bash (103) -- physical single-target, 130-430% multiplier -- **M**. Dep: 0.2.1 (`physicalSinglePattern`)
- [ ] **1.3.2** Magnum Break (105) -- physical AoE + fire endow self-buff -- **M**. Dep: 0.2.1 (`physicalAoePattern`), 1.2.1
- [ ] **1.3.3** Endure (106) -- self-buff: +MDEF, flinch immunity, 10-40s duration -- **M**. Dep: 0.2.1 (`selfBuffPattern`)
- [ ] **1.3.4** Sword Mastery (100) -- passive ATK bonus when wielding 1H sword/dagger -- **S**. Dep: Phase 0
- [ ] **1.3.5** 2H Sword Mastery (101) -- passive ATK bonus when wielding 2H sword -- **S**. Dep: 1.3.4
- [ ] **1.3.6** VFX for Bash, Magnum Break, Endure -- **M**. Dep: 1.3.1, 1.3.2, 1.3.3

#### 1.4 Archer Skill Handlers (3 remaining active)

- [ ] **1.4.1** Double Strafe (303) -- 2-hit ranged physical attack -- **M**. Dep: 0.2.1
- [ ] **1.4.2** Arrow Shower (304) -- ground AoE arrows, 5x5 splash -- **M**. Dep: 0.2.1 (`physicalAoePattern`)
- [ ] **1.4.3** Improve Concentration (302) -- self-buff: +AGI, +DEX, reveal hidden -- **M**. Dep: 0.2.1 (`selfBuffPattern`)
- [ ] **1.4.4** Owl's Eye (300) -- passive: +DEX per level -- **S**. Dep: Phase 0
- [ ] **1.4.5** Vulture's Eye (301) -- passive: +range per level -- **S**. Dep: Phase 0
- [ ] **1.4.6** VFX for Double Strafe, Arrow Shower, Improve Concentration -- **M**. Dep: 1.4.1, 1.4.2, 1.4.3

#### 1.5 Acolyte Skill Handlers (8 remaining active)

- [x] Heal (400) -- DONE
- [ ] **1.5.1** Blessing (402) -- single buff: +STR/DEX/INT -- **M**. Dep: 0.2.1
- [ ] **1.5.2** Increase AGI (403) -- single buff: +AGI -- **M**. Dep: 0.2.1
- [ ] **1.5.3** Decrease AGI (404) -- single debuff: -AGI -- **M**. Dep: 0.2.1
- [ ] **1.5.4** Cure (405) -- remove Poison, Silence, Blind, Curse -- **M**. Dep: 1.2.2, 1.2.7, 1.2.8, 1.2.9
- [ ] **1.5.5** Angelus (406) -- party buff: +VIT DEF -- **S**. Dep: 0.2.1 (party-wide effect deferred to Phase 6)
- [ ] **1.5.6** Signum Crucis (407) -- AoE debuff vs Undead/Demon -- **S**. Dep: 0.2.1
- [ ] **1.5.7** Ruwach (408) -- reveal hidden units in AoE -- **S**. Dep: 0.2.1
- [ ] **1.5.8** Teleport (409) -- random teleport or save-point return -- **M**. Dep: Phase 0 (zone flag checks)
- [ ] **1.5.9** Warp Portal (410) -- create ground portal for party -- **L**. Dep: 0.2.1 (needs portal entity system)
- [ ] **1.5.10** Pneuma (411) -- ground effect blocking ranged attacks -- **M**. Dep: 0.2.1
- [ ] **1.5.11** VFX for all Acolyte skills -- **L**. Dep: 1.5.1 through 1.5.10

#### 1.6 Thief Skill Handlers (4 remaining active)

- [ ] **1.6.1** Double Attack (500) -- passive: chance of double hit on auto-attack -- **M**. Dep: Phase 0
- [ ] **1.6.2** Envenom (504) -- physical + poison chance -- **M**. Dep: 0.2.1, 1.2.2
- [ ] **1.6.3** Steal (502) -- attempt steal from monster -- **M**. Dep: Phase 0 (needs steal formula)
- [ ] **1.6.4** Hiding (503) -- toggle invisibility (hidden state system) -- **L**. Dep: Phase 0 (needs hidden state)
- [ ] **1.6.5** Detoxify (505) -- cure poison -- **S**. Dep: 1.2.2
- [ ] **1.6.6** VFX for Envenom, Steal, Hiding, Detoxify -- **M**. Dep: 1.6.1 through 1.6.5

#### 1.7 Merchant Skill Handlers (2 remaining active)

- [ ] **1.7.1** Mammonite (603) -- physical attack + zeny cost -- **M**. Dep: 0.2.1 (`physicalSinglePattern`)
- [ ] **1.7.2** Enlarge Weight Limit (600) -- passive: +max weight -- **S**. Dep: Phase 0
- [ ] **1.7.3** VFX for Mammonite -- **S**. Dep: 1.7.1

#### 1.8 Element / Size / Race Integration

- [ ] **1.8.1** Wire monster template `element` and `elementLevel` into all damage calls -- **M**. Dep: Phase 0
- [ ] **1.8.2** Wire monster template `size` (Small/Medium/Large) into physical damage calls -- **M**. Dep: Phase 0
- [ ] **1.8.3** Wire monster template `race` into damage calls (for future card modifiers) -- **S**. Dep: Phase 0
- [ ] **1.8.4** Pass equipped `weaponType` from inventory to size penalty lookup -- **M**. Dep: Phase 0
- [ ] **1.8.5** Test element effectiveness: fire spell vs water monster = 2x, etc. -- **S**. Dep: 1.8.1

#### 1.9 Critical Hit Refinement

- [ ] **1.9.1** Ensure critical hits deal max ATK (not random variance) -- **S**. Dep: Phase 0
- [ ] **1.9.2** Add Katar double critical rate bonus -- **S**. Dep: Phase 0
- [ ] **1.9.3** Add critical hit visual indicator on client (color or animation) -- **S**. Dep: 1.9.1
- [ ] **1.9.4** Display Lucky Dodge in combat stats panel -- **S**. Dep: Phase 0

#### 1.10 Hidden State System

- [ ] **1.10.1** Server-side hidden flag on player state -- **M**. Dep: Phase 0
- [ ] **1.10.2** Hidden players not visible to enemies (skip aggro) -- **S**. Dep: 1.10.1
- [ ] **1.10.3** Hidden players not visible to other players (client hides) -- **S**. Dep: 1.10.1
- [ ] **1.10.4** Detection skills (Ruwach, Sight, Improve Concentration) reveal hidden -- **S**. Dep: 1.10.1
- [ ] **1.10.5** Actions break hidden state (attack, skill use, taking damage) -- **S**. Dep: 1.10.1

---

### PHASE 5: Items & Equipment Deep Dive

**Goal**: Expand items to 500+, implement refining, card compound, weight enforcement, full consumable effects.
**Guide**: `05_Items_Equipment_Cards.md`
**Dependencies**: Phase 1 (status effects for card procs)

#### 5.1 Item Database Expansion

- [ ] **5.1.1** Create script `scripts/generate_item_database.js` to extract items from rAthena item_db -- **L**
- [ ] **5.1.2** Add 130+ weapons covering all types (Dagger, 1H Sword, 2H Sword, Spear, Axe, Mace, Rod, Bow, Katar, Book, Knuckle) -- **L**. Dep: 5.1.1
- [ ] **5.1.3** Add 40+ armor pieces (body, shield, footgear, garment) -- **M**. Dep: 5.1.1
- [ ] **5.1.4** Add 90+ headgear (top/mid/low) -- **M**. Dep: 5.1.1
- [ ] **5.1.5** Add 40+ accessories -- **M**. Dep: 5.1.1
- [ ] **5.1.6** Add 30+ additional consumables with proper effect definitions -- **M**. Dep: 5.1.1
- [ ] **5.1.7** Add 140+ etc/material items (crafting, quest, refining materials) -- **M**. Dep: 5.1.1
- [ ] **5.1.8** Add 170+ additional cards -- **L**. Dep: 5.1.1
- [ ] **5.1.9** Create migration: `database/migrations/add_expanded_items.sql` -- **M**. Dep: 5.1.2 through 5.1.8

#### 5.2 Equipment Refining System

- [ ] **5.2.1** Add `refine_level` column to `character_inventory` -- **S**
- [ ] **5.2.2** Add `weapon_level` and `card_slots` columns to `items` table -- **S**
- [ ] **5.2.3** Implement refine success rate table (safe +1-4, risky +5-10) -- **M**
- [ ] **5.2.4** Implement `refine:attempt` / `refine:result` socket events -- **M**. Dep: 5.2.1, 5.2.2, 5.2.3
- [ ] **5.2.5** Calculate refine bonus on ATK/DEF in derived stats -- **M**. Dep: 5.2.1
- [ ] **5.2.6** Create refining NPC actor -- **M**. Dep: 5.2.4
- [ ] **5.2.7** Create `SRefineWidget` (Slate) for refine UI -- **M**. Dep: 5.2.4
- [ ] **5.2.8** VFX: refine success/failure animation -- **S**. Dep: 5.2.4

#### 5.3 Card Compound System

- [ ] **5.3.1** Add `card_slot_1` through `card_slot_4` columns to `character_inventory` -- **S**
- [ ] **5.3.2** Create `server/src/ro_card_effects.js` mapping card_id to effect functions -- **XL**
- [ ] **5.3.3** Implement compound action: insert card into equipment slot -- **M**. Dep: 5.3.1, 5.3.2
- [ ] **5.3.4** On equip, aggregate card stat bonuses into effective stats -- **M**. Dep: 5.3.2
- [ ] **5.3.5** On auto-attack, check for proc effects (5% stun, drain HP, auto-cast) -- **L**. Dep: 5.3.2, 1.2.1
- [ ] **5.3.6** Wire card racial/size/element damage bonuses into damage formula -- **M**. Dep: 5.3.2
- [ ] **5.3.7** Create `card:compound` / `card:compound_result` socket events -- **M**. Dep: 5.3.3
- [ ] **5.3.8** UI: compound interface in inventory (right-click card, select target equipment) -- **M**. Dep: 5.3.7

#### 5.4 Consumable Effect Enhancement

- [ ] **5.4.1** Implement HP potion tiers (Red, Orange, Yellow, White, Yggdrasil Berry) -- **M**
- [ ] **5.4.2** Implement SP potion tiers (Blue Potion, Grape Juice) -- **S**
- [ ] **5.4.3** Implement status cure items (Green Herb -> Poison, Panacea -> all) -- **M**. Dep: 1.2.1
- [ ] **5.4.4** Implement buff food (STR/AGI/VIT/INT/DEX/LUK foods, 30 min duration) -- **M**. Dep: 1.2.1
- [ ] **5.4.5** Implement speed potions (Awakening Potion: +ASPD) -- **S**. Dep: 1.2.1
- [x] Fly Wing / Butterfly Wing -- DONE

#### 5.5 Weight System Enforcement

- [ ] **5.5.1** Calculate total carried weight on every inventory change -- **M**
- [ ] **5.5.2** Max weight formula: 2000 + STR * 30 (+ class modifier) -- **S**. Dep: 5.5.1
- [ ] **5.5.3** Enforce 50% threshold: cannot use HP/SP regen items -- **M**. Dep: 5.5.1
- [ ] **5.5.4** Enforce 90% threshold: cannot move, cannot attack -- **M**. Dep: 5.5.1
- [ ] **5.5.5** Send weight status in `inventory:data` payload -- **S**. Dep: 5.5.1
- [ ] **5.5.6** Block item pickup if would exceed max weight -- **S**. Dep: 5.5.1
- [ ] **5.5.7** Display weight bar in inventory widget -- **S**. Dep: 5.5.5

#### 5.6 Equipment Visual Events

- [ ] **5.6.1** Add `player:equipment_visual` socket event broadcasting equipped item IDs -- **M**
- [ ] **5.6.2** Client-side mapping of item IDs to mesh/material assets (foundation for Phase 11) -- **M**. Dep: 5.6.1

---

### PHASE 2: Class System

**Goal**: Complete job change flow, class-specific mechanics, all second-class skill handlers.
**Guide**: `01_Stats_Leveling_JobSystem.md`, `03_Skills_Complete.md`
**Dependencies**: Phase 1 (first-class skills), Phase 5 (weapon types for restrictions)

#### 2.1 Class-Specific Mechanics

- [ ] **2.1.1** Implement per-class HP/SP modifier formulas -- **M**
- [ ] **2.1.2** Implement per-class weapon restrictions (equip validation server-side) -- **M**. Dep: Phase 5
- [ ] **2.1.3** Wire per-class per-weapon ASPD tables into derived stat calculation -- **M**. Dep: 1.1.1
- [ ] **2.1.4** Add class-specific base stat bonuses (Swordsman +30% HP, Mage +30% SP, etc.) -- **M**
- [ ] **2.1.5** Job Change NPC actors for all 6 classes placed in appropriate towns -- **M**. Dep: Phase 3 (for towns beyond Prontera)

#### 2.2 Second-Class Skill Handlers -- Tier 1 (Core Combat)

##### Knight (10 skills)
- [ ] **2.2.1** Pierce -- physical, extra damage vs Large -- **M**
- [ ] **2.2.2** Spear Stab -- physical + knockback -- **M**
- [ ] **2.2.3** Brandish Spear -- directional AoE physical -- **L**
- [ ] **2.2.4** Bowling Bash -- physical + knockback chain -- **L**
- [ ] **2.2.5** Two-Hand Quicken -- self-buff: +ASPD with 2H sword -- **M**
- [ ] **2.2.6** Auto Counter -- counter-attack buff -- **M**
- [ ] **2.2.7** Spear Boomerang -- ranged physical with spear -- **M**
- [ ] **2.2.8** Riding -- mount Peco Peco (links to Phase 10) -- **M**
- [ ] **2.2.9** Cavalry Mastery -- passive: reduce mount ASPD penalty -- **S**
- [ ] **2.2.10** VFX for all Knight skills -- **L**

##### Wizard (10 skills)
- [ ] **2.2.11** Jupitel Thunder -- multi-hit wind bolt + knockback -- **M**
- [ ] **2.2.12** Storm Gust -- massive ground AoE + freeze chance -- **L**
- [ ] **2.2.13** Meteor Storm -- random fire strikes in AoE + Stun -- **L**
- [ ] **2.2.14** Lord of Vermilion -- AoE wind + Blind chance -- **L**
- [ ] **2.2.15** Earth Spike -- earth single-target bolt -- **M**
- [ ] **2.2.16** Heaven's Drive -- ground AoE earth -- **M**
- [ ] **2.2.17** Water Ball -- multi-hit water, requires water terrain -- **M**
- [ ] **2.2.18** Ice Wall -- create wall of ice blocks -- **L**
- [ ] **2.2.19** Quagmire -- ground debuff: -AGI, -DEX -- **M**
- [ ] **2.2.20** Sight Rasher -- AoE fire from Sight -- **M**
- [ ] **2.2.21** VFX for all Wizard skills -- **XL**

##### Priest (8 skills)
- [ ] **2.2.22** Sanctuary -- ground AoE heal over time -- **L**
- [ ] **2.2.23** Kyrie Eleison -- barrier absorbing N hits -- **M**
- [ ] **2.2.24** Magnificat -- party buff: double SP regen -- **M**
- [ ] **2.2.25** Gloria -- party buff: +LUK -- **S**
- [ ] **2.2.26** Resurrection -- revive dead player -- **L**
- [ ] **2.2.27** Magnus Exorcismus -- ground AoE holy vs Undead/Demon -- **L**
- [ ] **2.2.28** Turn Undead -- instant kill chance vs Undead -- **M**
- [ ] **2.2.29** Lex Aeterna -- next hit deals double damage -- **M**
- [ ] **2.2.30** VFX for all Priest skills -- **L**

##### Assassin (7 skills)
- [ ] **2.2.31** Sonic Blow -- 8-hit physical burst -- **M**
- [ ] **2.2.32** Grimtooth -- ranged physical from hiding -- **M**. Dep: 1.10.1
- [ ] **2.2.33** Cloaking -- advanced hiding (can move while hidden near walls) -- **L**. Dep: 1.10.1
- [ ] **2.2.34** Poison React -- counter poison on being hit -- **M**
- [ ] **2.2.35** Venom Dust -- ground poison trap -- **M**
- [ ] **2.2.36** Sonic Acceleration -- passive: +HIT, +damage with Sonic Blow -- **S**
- [ ] **2.2.37** Katar Mastery -- passive: +ATK with Katar -- **S**
- [ ] **2.2.38** VFX for all Assassin skills -- **L**

##### Hunter (8 skills)
- [ ] **2.2.39** Blitz Beat -- falcon attack (auto-trigger + manual) -- **M**
- [ ] **2.2.40** Steel Crow -- passive: +falcon damage -- **S**
- [ ] **2.2.41** Detect -- reveal hidden in AoE -- **S**. Dep: 1.10.1
- [ ] **2.2.42** Ankle Snare -- ground trap: immobilize -- **M** (new Trap pattern)
- [ ] **2.2.43** Land Mine -- ground trap: damage + Stun -- **M**. Dep: 2.2.42
- [ ] **2.2.44** Remove Trap -- remove own trap -- **S**. Dep: 2.2.42
- [ ] **2.2.45** Shockwave Trap -- ground trap: drain SP -- **M**. Dep: 2.2.42
- [ ] **2.2.46** Claymore Trap -- ground trap: fire AoE damage -- **M**. Dep: 2.2.42
- [ ] **2.2.47** VFX for all Hunter skills -- **L**

#### 2.3 Second-Class Skill Handlers -- Tier 2 (Support/Utility)

##### Crusader (7 skills)
- [ ] **2.3.1** Holy Cross -- holy physical, 2 hits -- **M**
- [ ] **2.3.2** Grand Cross -- AoE holy+dark, costs HP -- **L**
- [ ] **2.3.3** Guard -- passive: chance to block damage -- **M**
- [ ] **2.3.4** Shield Charge -- physical + stun + knockback, requires shield -- **M**
- [ ] **2.3.5** Shield Boomerang -- ranged physical, requires shield -- **M**
- [ ] **2.3.6** Devotion -- take damage for party member -- **L**
- [ ] **2.3.7** Faith -- passive: +HP, +holy resistance -- **S**
- [ ] **2.3.8** VFX for all Crusader skills -- **L**

##### Monk (6 skills)
- [ ] **2.3.9** Iron Fists -- passive: +ATK bare-handed -- **S**
- [ ] **2.3.10** Summon Spirit Sphere -- create spirit sphere counter -- **M** (new Spirit Sphere pattern)
- [ ] **2.3.11** Occult Impaction -- physical ignoring DEF -- **M**
- [ ] **2.3.12** Investigate -- DEF-based damage -- **M**
- [ ] **2.3.13** Finger Offensive -- spend spheres for multi-hit -- **M**. Dep: 2.3.10
- [ ] **2.3.14** Asura Strike -- massive single hit, spend all SP + spheres -- **L**. Dep: 2.3.10
- [ ] **2.3.15** VFX for all Monk skills -- **L**

##### Blacksmith (6 skills)
- [ ] **2.3.16** Adrenaline Rush -- party buff: +ASPD with axe/mace -- **M**
- [ ] **2.3.17** Weapon Perfection -- ignore size penalty -- **M**
- [ ] **2.3.18** Power Thrust -- +ATK%, chance to break weapon -- **M**
- [ ] **2.3.19** Maximize Power -- max weapon ATK for duration -- **M**
- [ ] **2.3.20** Weaponry Research -- passive: +ATK, +HIT -- **S**
- [ ] **2.3.21** Skin Tempering -- passive: +fire/neutral resistance -- **S**
- [ ] **2.3.22** VFX for all Blacksmith skills -- **M**

#### 2.4 Second-Class Skill Handlers -- Tier 3 (Specialized)

##### Sage (5 skills)
- [ ] **2.4.1** Study -- passive: +INT per level -- **S**
- [ ] **2.4.2** Cast Cancel -- cancel own cast (recover partial SP) -- **M**
- [ ] **2.4.3** Hindsight -- auto-cast bolt skills on melee attack -- **L**
- [ ] **2.4.4** Dispell -- remove buffs from target -- **M**
- [ ] **2.4.5** Magic Rod -- absorb magic attack as SP -- **M**
- [ ] **2.4.6** VFX for all Sage skills -- **M**

##### Rogue (5 skills)
- [ ] **2.4.7** Snatcher -- passive: auto-steal on attack -- **M**. Dep: 1.6.3
- [ ] **2.4.8** Back Stab -- physical from behind, extra damage -- **M**
- [ ] **2.4.9** Tunnel Drive -- move while hiding -- **M**. Dep: 1.10.1
- [ ] **2.4.10** Raid -- AoE physical from hiding + stun/blind chance -- **M**. Dep: 1.10.1
- [ ] **2.4.11** Intimidate -- steal EXP + random teleport target -- **M**
- [ ] **2.4.12** VFX for all Rogue skills -- **M**

##### Alchemist (5 skills)
- [ ] **2.4.13** Pharmacy -- craft potions from materials -- **L**
- [ ] **2.4.14** Acid Terror -- ranged physical + chance to break armor -- **M**
- [ ] **2.4.15** Demonstration -- ground AoE fire + weapon break chance -- **M**
- [ ] **2.4.16** Summon Flora -- summon plant ally -- **L**
- [ ] **2.4.17** Axe Mastery -- passive: +ATK with axe -- **S**
- [ ] **2.4.18** VFX for all Alchemist skills -- **M**

##### Bard (3 skills)
- [ ] **2.4.19** Music Lessons -- passive: +ATK, +SP -- **S**
- [ ] **2.4.20** A Poem of Bragi -- AoE buff: -cast time for allies -- **L** (new Performance pattern)
- [ ] **2.4.21** Assassin Cross of Sunset -- AoE buff: +ASPD for allies -- **L**. Dep: 2.4.20

##### Dancer (3 skills)
- [ ] **2.4.22** Dance Lessons -- passive: +ATK, +SP -- **S**
- [ ] **2.4.23** Service For You -- AoE buff: -SP cost for allies -- **L**. Dep: 2.4.20
- [ ] **2.4.24** Humming -- AoE buff: +HIT for allies -- **L**. Dep: 2.4.20

#### 2.5 New Skill Pattern Systems

- [ ] **2.5.1** Trap system: ground-placed entity with trigger radius, duration, effect callback -- **L**. Dep: Phase 0
- [ ] **2.5.2** Spirit Sphere system: counter resource on Monk, consumed by skills -- **M**. Dep: Phase 0
- [ ] **2.5.3** Performance/Song system: AoE buff centered on caster while channeling, canceled on move -- **L**. Dep: Phase 0
- [ ] **2.5.4** Counter system: trigger on being hit, consume buff charge -- **M**. Dep: Phase 0
- [ ] **2.5.5** Resurrection target: target dead player, restore HP%, clear death state -- **M**. Dep: Phase 0

---

### PHASE 3: World Expansion

**Goal**: Grow from 4 zones to 15+ zones (4 towns + 11+ fields/dungeons).
**Guide**: `06_World_Maps_Zones.md`, `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md`
**Dependencies**: Phase 0 (zone infrastructure stable)

#### 3.1 Prontera Region Completion

- [x] `prontera` (town) -- DONE
- [x] `prontera_south` (field, Lv 1-15) -- DONE
- [x] `prontera_north` (field, Lv 10-20) -- DONE
- [x] `prt_dungeon_01` (dungeon, Lv 15-25) -- DONE
- [ ] **3.1.1** `prt_dungeon_02` (prt_sewb2, Lv 25-40) -- Level + zone registry + spawns + warps -- **L**
- [ ] **3.1.2** `prontera_east` (prt_fild02, Lv 10-20) -- Level + zone registry + spawns + warps -- **L**
- [ ] **3.1.3** `prontera_west` (prt_fild03, Lv 15-25) -- Level + zone registry + spawns + warps -- **L**

#### 3.2 Geffen Region

- [ ] **3.2.1** `geffen` (town) -- Level, Kafra NPC, warp portals, no enemies -- **L**
- [ ] **3.2.2** `geffen_field_01` (gef_fild00, Lv 20-30) -- **L**. Dep: 3.2.1
- [ ] **3.2.3** `geffen_field_02` (gef_fild01, Lv 25-35) -- **L**. Dep: 3.2.1
- [ ] **3.2.4** `geffen_tower_01` (gef_tower, Lv 30-40) -- **L**. Dep: 3.2.1
- [ ] **3.2.5** `geffen_tower_02` (gef_tower2, Lv 35-50) -- **L**. Dep: 3.2.4
- [ ] **3.2.6** `geffen_dungeon_01` (gef_dun00, Lv 50-65) -- **L**. Dep: 3.2.1

#### 3.3 Payon Region

- [ ] **3.3.1** `payon` (town) -- Level, Kafra NPC, warp portals -- **L**
- [ ] **3.3.2** `payon_field_01` (pay_fild01, Lv 15-25) -- **L**. Dep: 3.3.1
- [ ] **3.3.3** `payon_cave_01` (pay_dun00, Lv 25-35) -- **L**. Dep: 3.3.1
- [ ] **3.3.4** `payon_cave_02` (pay_dun01, Lv 30-45) -- **L**. Dep: 3.3.3
- [ ] **3.3.5** `payon_cave_03` (pay_dun02, Lv 40-55) -- **L**. Dep: 3.3.4

#### 3.4 Morroc Region

- [ ] **3.4.1** `morroc` (town) -- Level, Kafra NPC, warp portals -- **L**
- [ ] **3.4.2** `morroc_field_01` (moc_fild01, Lv 15-30) -- **L**. Dep: 3.4.1
- [ ] **3.4.3** `morroc_pyramids_01` (moc_pryd01, Lv 25-40) -- **L**. Dep: 3.4.1
- [ ] **3.4.4** `morroc_pyramids_02` (moc_pryd02, Lv 35-50) -- **L**. Dep: 3.4.3
- [ ] **3.4.5** `morroc_sphinx_01` (moc_sphinx1, Lv 50-65) -- **L**. Dep: 3.4.1

#### 3.5 Warp Portal Network

- [ ] **3.5.1** Define complete warp graph (all zone-to-zone connections) in `ro_zone_data.js` -- **M**. Dep: zones exist
- [ ] **3.5.2** Place WarpPortal actors in all town zones connecting to fields -- **M**. Dep: 3.5.1
- [ ] **3.5.3** Place WarpPortal actors in all field zones connecting to towns and dungeons -- **M**. Dep: 3.5.1
- [ ] **3.5.4** Place WarpPortal actors in all dungeon floors connecting up/down -- **M**. Dep: 3.5.1
- [ ] **3.5.5** Verify all warp connections work bidirectionally -- **M**. Dep: 3.5.2 through 3.5.4

#### 3.6 Kafra Service Expansion

- [ ] **3.6.1** Place Kafra NPCs in Geffen, Payon, Morroc -- **M**. Dep: towns built
- [ ] **3.6.2** Add teleport destinations to each Kafra (all town-to-town) -- **M**. Dep: 3.6.1
- [ ] **3.6.3** Add zeny cost per teleport based on distance -- **S**. Dep: 3.6.2

---

### PHASE 4: Monster System Expansion

**Goal**: 100+ spawn points, monster skills, MVP bosses, mini-bosses.
**Guide**: `04_Monsters_EnemyAI.md`
**Dependencies**: Phase 1 (status effects for monster skills), Phase 3 (zones for spawns)

#### 4.1 Monster Spawn Expansion

- [x] Zones 1-3: 46 spawns -- DONE
- [ ] **4.1.1** Activate zone 4-6 spawns (mid-level Lv 20-45 monsters) -- **M** per zone. Dep: corresponding Phase 3 zones
- [ ] **4.1.2** Activate zone 7-9 spawns (high-level Lv 45-80 monsters) -- **M** per zone. Dep: corresponding Phase 3 zones
- [ ] **4.1.3** Define spawn points for each new zone in zone registry -- **M** per zone. Dep: zones exist
- [ ] **4.1.4** Target: 100+ total spawn points across 15+ zones -- aggregate. Dep: 4.1.1, 4.1.2

#### 4.2 Monster Skill System

- [ ] **4.2.1** Create `server/src/ro_monster_skills.js` -- per-monster skill list with IDs, cast rates, conditions -- **L**
- [ ] **4.2.2** Add skill execution to AI combat tick (check HP threshold, target count, random chance) -- **L**. Dep: 4.2.1
- [ ] **4.2.3** Implement monster Heal (self-heal on HP < 50%) -- **M**. Dep: 4.2.2
- [ ] **4.2.4** Implement monster Fireball (AoE fire damage) -- **M**. Dep: 4.2.2
- [ ] **4.2.5** Implement monster Teleport (random position when HP < 30%) -- **M**. Dep: 4.2.2
- [ ] **4.2.6** Implement monster Stone Curse -- **M**. Dep: 4.2.2, 1.2.5
- [ ] **4.2.7** Implement monster Frost Diver -- **M**. Dep: 4.2.2
- [ ] **4.2.8** Implement monster Summon (spawn minion monsters) -- **L**. Dep: 4.2.2
- [ ] **4.2.9** Add `enemy:skill_used` / `enemy:cast_start` socket events -- **M**. Dep: 4.2.2
- [ ] **4.2.10** Client VFX for monster skill usage -- **L**. Dep: 4.2.9

#### 4.3 MVP Boss System

- [ ] **4.3.1** MVP spawn timer system (respawn 1-2hr after death, random offset) -- **L**
- [ ] **4.3.2** MVP spawn announcement (zone-wide or server-wide) -- **M**. Dep: 4.3.1
- [ ] **4.3.3** MVP tombstone (visual marker at death location with killer name) -- **M**
- [ ] **4.3.4** MVP drop table (separate from normal drops, only MVP killer gets) -- **M**. Dep: 4.3.1
- [ ] **4.3.5** Implement Golden Bug MVP (Lv 45, Sewers B4, reflect magic) -- **L**. Dep: 4.3.1, 4.2.2
- [ ] **4.3.6** Implement Phreeoni MVP (Lv 53, Morroc Fields) -- **L**. Dep: 4.3.1, 4.2.2
- [ ] **4.3.7** Implement Baphomet MVP (Lv 81, Hidden Temple) -- **L**. Dep: 4.3.1, 4.2.2
- [ ] **4.3.8** MVP phase mechanics (HP threshold skill unlocks, enrage) -- **L**. Dep: 4.3.5

#### 4.4 Mini-Boss System

- [ ] **4.4.1** Implement Angeling mini-boss (Lv 45, fields) -- **M**
- [ ] **4.4.2** Implement Deviling mini-boss (Lv 50, fields) -- **M**
- [ ] **4.4.3** Implement Mastering mini-boss (Lv 25, fields) -- **M**
- [ ] **4.4.4** Implement Ghostring mini-boss (Lv 40, dungeons) -- **M**
- [ ] **4.4.5** Mini-boss spawn timers (shorter than MVP, ~30 min) -- **M**

#### 4.5 Drop Table Verification

- [ ] **4.5.1** Verify all 846 resolved drops match RO classic data -- **L**
- [ ] **4.5.2** Implement guaranteed drops (100% chance items like Jellopy from Poring) -- **M**
- [ ] **4.5.3** Card drop rate (0.01% base, LUK modifier) -- **M**
- [ ] **4.5.4** Server-wide drop rate multiplier events (2x, 3x) -- **M**
- [ ] **4.5.5** Thief Steal interaction with drop tables -- **M**. Dep: 1.6.3

---

### PHASE 6: Social Systems

**Goal**: Party, guild, chat expansion, friend list, block system.
**Guide**: `08_PvP_Guild_WoE.md`, `11_Multiplayer_Networking.md`
**Dependencies**: Phase 1 (combat for party EXP share)

#### 6.1 Party System

- [ ] **6.1.1** DB: Create `parties` and `party_members` tables -- **S**
- [ ] **6.1.2** Server: `party:create` handler (name, leader) -- **M**. Dep: 6.1.1
- [ ] **6.1.3** Server: `party:invite` / `party:accept` / `party:reject` handlers -- **M**. Dep: 6.1.2
- [ ] **6.1.4** Server: `party:leave` / `party:kick` / `party:disband` handlers -- **M**. Dep: 6.1.2
- [ ] **6.1.5** Server: `party:data` broadcast (member list with HP/SP/zone) -- **M**. Dep: 6.1.2
- [ ] **6.1.6** Server: `party:member_update` periodic broadcast (HP/SP/position) -- **M**. Dep: 6.1.5
- [ ] **6.1.7** Server: Party EXP share (even_share and each_take modes) -- **L**. Dep: 6.1.2
- [ ] **6.1.8** Server: Party leader transfer -- **S**. Dep: 6.1.2
- [ ] **6.1.9** Server: Cross-zone party persistence -- **M**. Dep: 6.1.2
- [ ] **6.1.10** Client: `PartySubsystem` (UWorldSubsystem) -- **L**. Dep: 6.1.5
- [ ] **6.1.11** Client: `SPartyWidget` (Slate) -- member list with HP bars, status -- **L**. Dep: 6.1.10
- [ ] **6.1.12** Upgrade party-scope skills (Angelus, Blessing, Magnificat) to target party members -- **M**. Dep: 6.1.7

#### 6.2 Guild System

- [ ] **6.2.1** DB: Create `guilds`, `guild_members`, `guild_storage` tables -- **M**
- [ ] **6.2.2** Server: `guild:create` handler (requires Emperium + 10,000 zeny) -- **M**. Dep: 6.2.1
- [ ] **6.2.3** Server: `guild:invite` / `guild:accept` / `guild:reject` handlers -- **M**. Dep: 6.2.2
- [ ] **6.2.4** Server: `guild:leave` / `guild:kick` / `guild:disband` handlers -- **M**. Dep: 6.2.2
- [ ] **6.2.5** Server: Guild ranks (1-20 with customizable names) -- **M**. Dep: 6.2.2
- [ ] **6.2.6** Server: `guild:data` broadcast (member list with rank, online status) -- **M**. Dep: 6.2.2
- [ ] **6.2.7** Server: Guild notice/announcement board -- **S**. Dep: 6.2.2
- [ ] **6.2.8** Client: `GuildSubsystem` (UWorldSubsystem) -- **L**. Dep: 6.2.6
- [ ] **6.2.9** Client: `SGuildWidget` (Slate) -- member list, ranks, notice -- **L**. Dep: 6.2.8

#### 6.3 Chat Expansion

- [x] Global chat -- DONE
- [ ] **6.3.1** Zone chat (same zone only) -- **M**
- [ ] **6.3.2** Party chat channel -- **M**. Dep: 6.1.2
- [ ] **6.3.3** Guild chat channel -- **M**. Dep: 6.2.2
- [ ] **6.3.4** Whisper (direct player-to-player) -- **M**
- [ ] **6.3.5** Trade chat channel -- **S**
- [ ] **6.3.6** System announcement channel -- **S**
- [ ] **6.3.7** Client: Chat tabs/channels in WBP_ChatWidget -- **M**. Dep: 6.3.1 through 6.3.6

#### 6.4 Friend List

- [ ] **6.4.1** DB: Create `friend_list` table (character_id, friend_character_id, status) -- **S**
- [ ] **6.4.2** Server: `friend:add` / `friend:accept` / `friend:remove` handlers -- **M**. Dep: 6.4.1
- [ ] **6.4.3** Server: Online/offline status tracking for friends -- **M**. Dep: 6.4.2
- [ ] **6.4.4** Client: `FriendListSubsystem` + `SFriendListWidget` -- **L**. Dep: 6.4.3

#### 6.5 Block/Ignore System

- [ ] **6.5.1** DB: Create `block_list` table -- **S**
- [ ] **6.5.2** Server: Block prevents whispers and trade requests -- **M**. Dep: 6.5.1
- [ ] **6.5.3** Client: Block option in context menu -- **S**. Dep: 6.5.2

---

### PHASE 8: NPC & Quest System

**Goal**: NPC dialogue engine, quest tracking, job change quests, access quests.
**Guide**: `07_NPCs_Quests_Shops.md`
**Dependencies**: Phase 3 (zones for NPC placement), Phase 2 (job change NPCs)

#### 8.1 NPC Dialogue Engine

- [ ] **8.1.1** Design JSON dialogue tree format (nodes, choices, conditions, actions) -- **M**
- [ ] **8.1.2** Server: `npc:talk` / `npc:choice` handlers -- **L**. Dep: 8.1.1
- [ ] **8.1.3** Server: Condition evaluators (level, class, items, quest state) -- **M**. Dep: 8.1.2
- [ ] **8.1.4** Server: Action executors (give item, take item, give EXP, start quest, warp) -- **M**. Dep: 8.1.2
- [ ] **8.1.5** Client: `DialogueSubsystem` (UWorldSubsystem) -- **L**. Dep: 8.1.2
- [ ] **8.1.6** Client: `SDialogueWidget` (Slate) -- NPC portrait, text, choice buttons -- **L**. Dep: 8.1.5

#### 8.2 Quest System

- [ ] **8.2.1** DB: Create `quest_definitions` and `character_quests` tables -- **M**
- [ ] **8.2.2** Server: `quest:start` / `quest:progress` / `quest:complete` handlers -- **L**. Dep: 8.2.1
- [ ] **8.2.3** Server: Objective types (Kill X, Collect X, Talk to NPC, Reach zone, Reach level) -- **L**. Dep: 8.2.2
- [ ] **8.2.4** Server: Reward distribution (EXP, zeny, items, skill points) -- **M**. Dep: 8.2.2
- [ ] **8.2.5** Client: `QuestSubsystem` (UWorldSubsystem) -- **L**. Dep: 8.2.2
- [ ] **8.2.6** Client: `SQuestLogWidget` (Slate) -- active/completed quest list -- **L**. Dep: 8.2.5
- [ ] **8.2.7** Quest NPC markers (! for new quest, ? for turn-in) -- **M**. Dep: 8.2.5

#### 8.3 Job Change Quests

- [ ] **8.3.1** Swordsman job change quest (Izlude) -- **L**. Dep: 8.2.2
- [ ] **8.3.2** Mage job change quest (Geffen) -- **L**. Dep: 8.2.2
- [ ] **8.3.3** Archer job change quest (Payon) -- **L**. Dep: 8.2.2
- [ ] **8.3.4** Acolyte job change quest (Prontera Church) -- **L**. Dep: 8.2.2
- [ ] **8.3.5** Thief job change quest (Morroc) -- **L**. Dep: 8.2.2
- [ ] **8.3.6** Merchant job change quest (Alberta) -- **L**. Dep: 8.2.2

#### 8.4 Access Quests

- [ ] **8.4.1** Prontera Culvert access quest (Lv 15+) -- **M**. Dep: 8.2.2
- [ ] **8.4.2** Geffen Tower access quest (Lv 30+) -- **M**. Dep: 8.2.2
- [ ] **8.4.3** Payon Cave access quest (Lv 20+) -- **M**. Dep: 8.2.2

---

### PHASE 9: Economy

**Goal**: Player trading, vending, Kafra storage, buying store.
**Guide**: `13_Economy_Trading_Vending.md`
**Dependencies**: Phase 5 (items), Phase 6 (social for trade context)

#### 9.1 Trading System

- [ ] **9.1.1** Server: `trade:request` / `trade:accept` / `trade:reject` handlers -- **M**
- [ ] **9.1.2** Server: `trade:add_item` / `trade:remove_item` / `trade:set_zeny` handlers -- **M**. Dep: 9.1.1
- [ ] **9.1.3** Server: `trade:lock` / `trade:confirm` / `trade:cancel` with anti-scam (re-lock on change) -- **L**. Dep: 9.1.2
- [ ] **9.1.4** Server: Atomic transaction (swap items + zeny in single DB transaction) -- **L**. Dep: 9.1.3
- [ ] **9.1.5** Client: `TradeSubsystem` + `STradeWidget` -- **L**. Dep: 9.1.3

#### 9.2 Vending System

- [ ] **9.2.1** Server: `vending:open` handler (Merchant places items with prices) -- **M**
- [ ] **9.2.2** Server: `vending:browse` / `vending:data` handlers -- **M**. Dep: 9.2.1
- [ ] **9.2.3** Server: `vending:buy` / `vending:sold` handlers -- **M**. Dep: 9.2.2
- [ ] **9.2.4** Server: `vending:close` handler -- **S**. Dep: 9.2.1
- [ ] **9.2.5** Vendor display (shop sign above character) -- **M**. Dep: 9.2.1
- [ ] **9.2.6** Client: `VendingSubsystem` + `SVendingWidget` -- **L**. Dep: 9.2.3

#### 9.3 Storage System

- [ ] **9.3.1** DB: Create `kafra_storage` table (user_id scope, 600 slots) -- **M**
- [ ] **9.3.2** Server: `storage:open` / `storage:data` / `storage:store` / `storage:retrieve` handlers -- **L**. Dep: 9.3.1
- [ ] **9.3.3** Server: `storage:deposit_zeny` / `storage:withdraw_zeny` handlers -- **M**. Dep: 9.3.1
- [ ] **9.3.4** Wire storage access through Kafra NPC (100 zeny fee) -- **S**. Dep: 9.3.2
- [ ] **9.3.5** Client: `StorageSubsystem` + `SStorageWidget` -- **L**. Dep: 9.3.2
- [ ] **9.3.6** Guild storage (guild_id scope, 600 slots) -- **L**. Dep: 6.2.1, 9.3.1

#### 9.4 Buying Store

- [ ] **9.4.1** Server: `buystore:open` / `buystore:sell_to` / `buystore:close` handlers -- **M**
- [ ] **9.4.2** Client: `SBuyingStoreWidget` -- **M**. Dep: 9.4.1

---

### PHASE 10: Companion Systems

**Goal**: Pets, Homunculus, Falcon, Peco Peco mount, Merchant cart.
**Guide**: `12_Pets_Homunculus_Companions.md`
**Dependencies**: Phase 2 (class-specific systems), Phase 5 (taming/food items)

#### 10.1 Pet System

- [ ] **10.1.1** DB: Create `character_pets` table (pet type, name, hunger, intimacy, level) -- **M**
- [ ] **10.1.2** Server: Taming logic (use taming item on monster, success chance) -- **M**. Dep: 10.1.1
- [ ] **10.1.3** Server: Pet hunger/feeding system -- **M**. Dep: 10.1.1
- [ ] **10.1.4** Server: Pet intimacy tracking, loyalty bonuses -- **M**. Dep: 10.1.3
- [ ] **10.1.5** Server: Pet follow AI (follow owner, idle near owner) -- **L**. Dep: 10.1.1
- [ ] **10.1.6** Define 6 initial tameable pets (Poring, Lunatic, Drops, Poporing, Chonchon, Spore) -- **M**
- [ ] **10.1.7** Client: `PetSubsystem` + pet visual actor -- **L**. Dep: 10.1.5
- [ ] **10.1.8** Client: `SPetWidget` (Slate) -- pet status, feed button, equip slot -- **M**. Dep: 10.1.7

#### 10.2 Falcon System (Hunter)

- [ ] **10.2.1** Server: Falcon acquisition via NPC (2,500 zeny) -- **M**
- [ ] **10.2.2** Server: Auto Blitz Beat trigger on auto-attack (INT-based chance) -- **M**. Dep: 10.2.1
- [ ] **10.2.3** Server: Steel Crow passive integration -- **S**. Dep: 10.2.1
- [ ] **10.2.4** Client: Falcon visual (perch on shoulder, fly during Blitz Beat) -- **L**. Dep: 10.2.1

#### 10.3 Peco Peco Mount (Knight/Crusader)

- [ ] **10.3.1** Server: Mount state on player (requires Riding skill + NPC) -- **M**
- [ ] **10.3.2** Server: +25% movement speed, -25% ASPD (reduced by Cavalry Mastery) -- **M**. Dep: 10.3.1
- [ ] **10.3.3** Server: Brandish Spear requires mount -- **S**. Dep: 10.3.1
- [ ] **10.3.4** Client: `UMountComponent` on player actor -- **L**. Dep: 10.3.1
- [ ] **10.3.5** Client: Mount visual (character rides Peco mesh) -- **L**. Dep: 10.3.4

#### 10.4 Cart System (Merchant)

- [ ] **10.4.1** Server: Cart acquisition via Kafra (requires Pushcart skill) -- **M**
- [ ] **10.4.2** Server: Cart storage (100 extra slots) -- **L**. Dep: 10.4.1
- [ ] **10.4.3** Server: -50% movement speed (reduced by Pushcart level) -- **M**. Dep: 10.4.1
- [ ] **10.4.4** Server: Vending requires cart -- **S**. Dep: 10.4.1, 9.2.1
- [ ] **10.4.5** Client: Cart visual (dragged behind character) -- **L**. Dep: 10.4.1

#### 10.5 Homunculus (Alchemist) -- Basic Framework

- [ ] **10.5.1** DB: Create `character_homunculus` table -- **M**
- [ ] **10.5.2** Server: Creation (Alchemist skill, requires materials) -- **M**. Dep: 10.5.1
- [ ] **10.5.3** Server: Homunculus AI (autonomous combat, follow owner, configurable aggression) -- **XL**. Dep: 10.5.2
- [ ] **10.5.4** Server: Homunculus leveling (independent EXP) -- **L**. Dep: 10.5.2
- [ ] **10.5.5** Implement 1 type fully: Lif (heal support) -- **L**. Dep: 10.5.3
- [ ] **10.5.6** Client: Homunculus actor + UI -- **L**. Dep: 10.5.5

---

### PHASE 7: PvP & War of Emperium

**Goal**: PvP zones, PvP rules, guild-vs-guild, War of Emperium castle siege.
**Guide**: `08_PvP_Guild_WoE.md`
**Dependencies**: Phase 6 (guild system), Phase 2 (all classes playable)

#### 7.1 PvP System

- [ ] **7.1.1** Add `pvp: true` flag to zone registry for PvP zones -- **S**
- [ ] **7.1.2** Server: PvP damage rules (60-70% of PvE damage, no EXP loss on death) -- **M**
- [ ] **7.1.3** Server: PvP kill/death tracking per character -- **M**
- [ ] **7.1.4** Create PvP arena zone(s) -- **L**. Dep: 7.1.1
- [ ] **7.1.5** Client: PvP ranking board widget -- **M**. Dep: 7.1.3

#### 7.2 War of Emperium

- [ ] **7.2.1** Create castle zone(s) with Emperium crystal actor -- **XL**
- [ ] **7.2.2** Server: WoE schedule system (configurable days/times) -- **L**
- [ ] **7.2.3** Server: Emperium mechanics (physical only, no heal, no miss, no crit, Holy 1) -- **L**. Dep: 7.2.1
- [ ] **7.2.4** Server: Castle ownership transfer on Emperium break -- **L**. Dep: 7.2.3
- [ ] **7.2.5** Server: Guardian NPC defenders -- **L**. Dep: 7.2.1
- [ ] **7.2.6** Server: Treasure box system (daily spawn for owning guild) -- **M**. Dep: 7.2.4
- [ ] **7.2.7** Server: WoE rules (no Teleport/Fly Wing, reduced items, reduced skill duration) -- **M**. Dep: 7.2.2
- [ ] **7.2.8** Server: Emergency Call skill (teleport guild to leader) -- **M**. Dep: 6.2.2
- [ ] **7.2.9** Client: WoE status display, castle ownership indicator -- **L**. Dep: 7.2.4

---

### PHASE 11: Art & Polish

**Goal**: Replace placeholders with stylized assets, RO aesthetic in 3D.
**Guide**: `10_Art_Animation_VFX_Pipeline.md`
**Dependencies**: All gameplay systems (art wraps around mechanics)

#### 11.1 Character Models

- [ ] **11.1.1** Base body mesh (male/female) with RO proportions -- **XL**
- [ ] **11.1.2** 19 hair styles per gender -- **XL**. Dep: 11.1.1
- [ ] **11.1.3** 9 hair color variations per style -- **L**. Dep: 11.1.2
- [ ] **11.1.4** Animation set: idle, walk, run, attack (per weapon), sit, dead, cast, emote -- **XL**. Dep: 11.1.1

#### 11.2 Monster Models

- [ ] **11.2.1** Starter monsters (Lv 1-15): 15-20 models + animations -- **XL**
- [ ] **11.2.2** Mid-level monsters (Lv 15-40): 20-30 models -- **XL**
- [ ] **11.2.3** High-level monsters (Lv 40-70): 20-30 models -- **XL**
- [ ] **11.2.4** MVP bosses: 10+ high-quality models -- **XL**

#### 11.3 Environment Art

- [ ] **11.3.1** Town art (buildings, plazas, fountains, signs) for Prontera, Geffen, Payon, Morroc -- **XL**
- [ ] **11.3.2** Field art (terrain, trees, rocks, grass, water) -- **XL**
- [ ] **11.3.3** Dungeon art (cave walls, torches, atmospheric lighting) -- **XL**

#### 11.4 Equipment Visuals

- [ ] **11.4.1** 5+ weapon mesh types visible on character -- **L**. Dep: 11.1.1
- [ ] **11.4.2** 10+ headgear meshes visible on character -- **L**. Dep: 11.1.1
- [ ] **11.4.3** Shield mesh on arm -- **M**. Dep: 11.1.1
- [ ] **11.4.4** Armor material swaps -- **M**. Dep: 11.1.1

#### 11.5 VFX Expansion

- [ ] **11.5.1** Physical attack VFX (slash, stab, blunt impact) -- **L**
- [ ] **11.5.2** VFX for all remaining skills (70+) -- **XL**
- [ ] **11.5.3** Status effect VFX for all 10 statuses -- **L**. Dep: 1.2.15
- [ ] **11.5.4** Environmental VFX (weather, ambient particles) -- **L**

#### 11.6 UI Art Polish

- [ ] **11.6.1** RO-style window frames (brown wood) -- **L**
- [ ] **11.6.2** Custom buttons with hover/press states -- **M**
- [ ] **11.6.3** Item/skill icons (24x24, hand-drawn style) -- **XL**
- [ ] **11.6.4** Custom cursor -- **S**
- [ ] **11.6.5** Minimap system -- **L**

---

### PHASE 12: Audio

**Goal**: BGM, combat SFX, skill SFX, UI sounds, ambient audio.
**Guide**: `14_Audio_Music_SFX.md`
**Dependencies**: Phase 3 (zones for BGM), Phase 1 (skills for SFX)

#### 12.1 Audio Infrastructure

- [ ] **12.1.1** Create `AudioSubsystem` (UWorldSubsystem) -- BGM playback, crossfade on zone change -- **L**
- [ ] **12.1.2** Volume controls (Master, BGM, SFX, Ambient) saved to GameUserSettings -- **M**. Dep: 12.1.1
- [ ] **12.1.3** 3D spatialization setup (attenuation curves, occlusion) -- **M**. Dep: 12.1.1

#### 12.2 BGM

- [ ] **12.2.1** Login screen BGM -- **S**
- [ ] **12.2.2** Town BGMs (Prontera, Geffen, Payon, Morroc) -- **M**
- [ ] **12.2.3** Field/dungeon BGMs -- **M**
- [ ] **12.2.4** Boss fight BGM -- **S**

#### 12.3 Sound Effects

- [ ] **12.3.1** Weapon attack SFX (10 types: slash, stab, blunt, arrow, spell) -- **M**
- [ ] **12.3.2** Skill SFX (30+ per skill) -- **XL**
- [ ] **12.3.3** Hit impact SFX (flesh, armor, critical, miss) -- **M**
- [ ] **12.3.4** UI SFX (button click, window open/close, equip, level up, error) -- **M**
- [ ] **12.3.5** Ambient per zone type (town, field, forest, dungeon, desert) -- **L**

---

### PHASE 13: Optimization & Launch Prep

**Goal**: 100+ concurrent players, security hardening, monitoring.
**Guide**: `11_Multiplayer_Networking.md`
**Dependencies**: All gameplay systems substantially complete

#### 13.1 Server Optimization

- [ ] **13.1.1** Implement interest management (broadcastToNearby instead of broadcastToZone) -- **XL**
- [ ] **13.1.2** Spatial partitioning for combat tick (skip distant enemies) -- **L**
- [ ] **13.1.3** Zone-based enemy activation (sleep inactive zones) -- **M**
- [ ] **13.1.4** Connection pool tuning based on load test results -- **M**
- [ ] **13.1.5** Object pooling server-side (reuse enemy/player state objects) -- **L**

#### 13.2 Client Optimization

- [ ] **13.2.1** LOD system for distant characters (reduce mesh complexity) -- **L**
- [ ] **13.2.2** Particle LOD (reduce/disable off-screen particles) -- **M**
- [ ] **13.2.3** Mesh instancing for similar enemies -- **L**
- [ ] **13.2.4** Streaming levels and texture streaming -- **L**
- [ ] **13.2.5** Async loading for zone transitions -- **M**

#### 13.3 Load Testing

- [ ] **13.3.1** 100 concurrent login test -- **L**
- [ ] **13.3.2** 100 concurrent players in one zone -- **L**
- [ ] **13.3.3** 500 total concurrent (spread across zones) -- **L**
- [ ] **13.3.4** MVP boss fight with 20 attackers -- **M**
- [ ] **13.3.5** 50 concurrent trades -- **M**

#### 13.4 Security Hardening

- [ ] **13.4.1** Rate limiting on REST + Socket.io events -- **M**
- [ ] **13.4.2** Position validation (reject speed hacking) -- done in Phase 0 (0.5.1)
- [ ] **13.4.3** AFK auto-disconnect (30 min inactivity) -- **M**
- [ ] **13.4.4** Multi-boxing detection (same IP limits, configurable) -- **M**
- [ ] **13.4.5** Packet replay protection (nonce on critical events) -- **L**

#### 13.5 Network Optimization

- [ ] **13.5.1** Delta compression for position updates -- **L**
- [ ] **13.5.2** Update throttling (10 Hz distant, 30 Hz nearby) -- **L**
- [ ] **13.5.3** Event batching (combine small events) -- **M**
- [ ] **13.5.4** Consider MessagePack for hot-path events -- **L**

#### 13.6 Monitoring

- [ ] **13.6.1** Server health dashboard (CPU, memory, connections, tick time) -- **L**
- [ ] **13.6.2** Database monitoring (query times, connection pool stats) -- **M**
- [ ] **13.6.3** Client performance metrics logging -- **M**

---

## 5. Per-Phase Milestone Definitions

Each milestone has specific, testable criteria. A phase is NOT complete until ALL criteria pass.

### Alpha 0.1 -- Phases 0 + 1

**"Core combat works with all first-class skills across 4 zones"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | Server starts from modularized codebase without errors | `npm run dev` succeeds, no crash on startup |
| 2 | All 15 previously-working skills still function identically | Manual test: cast each skill, verify damage/effect |
| 3 | All 6 first-class trees have working active skill handlers | Cast each new skill: Bash, Double Strafe, Blessing, Envenom, Mammonite, etc. Verify server responds |
| 4 | Status effect system supports all 10 core statuses | Apply each status, verify duration, tick effects, removal |
| 5 | ASPD uses per-class per-weapon lookup table | Equip different weapons, verify ASPD changes per-class |
| 6 | Element modifiers applied to all damage | Fire spell vs Water monster = 2x damage |
| 7 | Size penalties applied to physical damage | Wrong weapon vs wrong size = reduced damage |
| 8 | Critical hits deal correct damage with visual | Lucky hit shows distinct color, deals max ATK |
| 9 | Passive skills modify stats correctly | Sword Mastery adds ATK, Owl's Eye adds DEX |
| 10 | 2+ players can fight same target simultaneously | Two PIE instances attacking one monster, both see damage |
| 11 | Performance baseline document exists with numbers | File exists at `docsNew/05_Development/Performance_Baseline.md` |
| 12 | Position validation rejects teleporting | Speed hack attempt is corrected server-side |

### Alpha 0.2 -- Phases 0 + 1 + 2 + 5

**"All classes playable with items/equipment/refining complete"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | All 6 first-class job changes work via NPC | Talk to each class NPC, change job, verify skills unlock |
| 2 | All 13 second-class job changes work | First-class to second-class transition, verify new skills |
| 3 | Class-specific weapon restrictions enforced | Mage cannot equip sword, Archer cannot equip mace |
| 4 | All Tier 1 second-class skills have working handlers | Knight/Wizard/Priest/Assassin/Hunter -- test each skill |
| 5 | All Tier 2 second-class skills have working handlers | Crusader/Monk/Blacksmith -- test each skill |
| 6 | 500+ items in database | `SELECT COUNT(*) FROM items` returns 500+ |
| 7 | Refining system works (success and failure) | Refine weapon to +5, verify ATK increase; refine to +8, verify failure possible |
| 8 | Card compound system works | Insert card into weapon, verify stat bonus |
| 9 | Weight system enforced at 50% and 90% | Fill inventory, verify item use blocked at 50%, movement blocked at 90% |
| 10 | All consumable types have effects | Use potion, buff food, status cure -- all work |
| 11 | Trap system works for Hunter | Place Ankle Snare, monster walks into it, gets immobilized |
| 12 | Spirit sphere works for Monk | Summon spheres, use Finger Offensive, spheres consumed |

### Alpha 0.3 -- Phases 0-6

**"Social systems work, 15+ zones, diverse monster variety"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | 15+ zones playable | Count distinct zone entries in ZONE_REGISTRY |
| 2 | All warp portals connect correctly | Walk through each portal, verify arrival zone + position |
| 3 | 100+ monster spawn points | Count spawn entries across all zones |
| 4 | Monster skills work (3+ types) | Observe monster casting Heal, Fireball, Teleport |
| 5 | 3+ MVP bosses functional | Kill MVP, verify tombstone, special drops |
| 6 | Party system fully works | Create party, invite, join, share EXP on kill |
| 7 | Guild system fully works | Create guild, invite, join, set ranks |
| 8 | All chat channels work | Send message in zone, party, guild, whisper, trade |
| 9 | Friend list with online status | Add friend, verify online indicator |
| 10 | Kafra in all towns | Use Kafra in Geffen, Payon, Morroc -- save + teleport |

### Beta 0.5 -- Phases 0-9

**"Full gameplay loop: level, quest, trade, PvP"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | NPC dialogue engine works | Talk to quest NPC, navigate dialogue tree, make choices |
| 2 | All 6 job change quests implemented | Complete each job change quest from start to finish |
| 3 | 3+ access quests implemented | Complete Culvert/Tower/Cave access quests |
| 4 | Quest log shows active and completed quests | Open quest log, verify tracking |
| 5 | Player-to-player trading works | Two players trade items + zeny successfully |
| 6 | Vending system works | Merchant opens shop, another player buys from it |
| 7 | Kafra storage works | Deposit items, zone change, retrieve items |
| 8 | PvP zones functional | Two players fight in PvP zone, damage reduced, no EXP loss |
| 9 | A complete leveling path exists (1-99) | A player can level from 1 to 99 through available zones |

### Beta 0.8 -- Phases 0-12

**"Art polished, audio complete, companions"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | Player models with hair variety | See different hair styles in-game |
| 2 | 20+ monster models | Count distinct monster meshes |
| 3 | Equipment visible on character | Equip weapon, headgear -- verify visual change |
| 4 | BGM per zone | Enter each zone, hear distinct music |
| 5 | Combat SFX | Attack, skill, death -- hear sounds |
| 6 | Pet system works | Tame pet, feed, see follow |
| 7 | Mount system works | Knight mounts Peco, speed increases |
| 8 | WoE basic flow | Schedule WoE, break Emperium, castle ownership transfers |

### Release 1.0 -- Phases 0-13

**"Optimized, secure, launch-ready"**

| # | Criterion | How to Test |
|---|-----------|-------------|
| 1 | 100 concurrent players without server degradation | Load test passes all targets |
| 2 | 60 FPS with 50 on-screen characters | Client performance benchmark |
| 3 | No critical security vulnerabilities | Security audit checklist complete |
| 4 | Anti-cheat blocks speed hacking | Attempt speed hack, verify rejection |
| 5 | Monitoring dashboard operational | Server health visible in dashboard |
| 6 | Network bandwidth < 50 KB/s per client at full load | Bandwidth measurement tool |
| 7 | All gameplay systems functional and tested | Full regression test suite passes |
| 8 | Database optimized with indexes and monitoring | Query time p95 < 10ms |

---

## 6. Implementation Guide Index

These companion documents provide deep-dive specifications for each system. They are stored at `C:\Sabri_MMO\RagnaCloneDocs\` and should be read before starting work on their corresponding phase.

| # | Document | Phases | Covers | Read Before |
|---|----------|--------|--------|-------------|
| 01 | `01_Stats_Leveling_JobSystem.md` | 1, 2 | Base/job stats, EXP tables, stat allocation, class progression, ASPD per-class tables, HP/SP per-class formulas | Modifying stat formulas, adding class mechanics, ASPD work |
| 02 | `02_Combat_System.md` | 1 | Physical/magical damage formulas, HIT/FLEE, critical system, element table, size penalties, DEF/MDEF, auto-attack loop, skill damage pipeline | Any combat-related changes, damage formula modifications |
| 03 | `03_Skills_Complete.md` | 1, 2 | All 86+ skill definitions, cast times, cooldowns, SP costs, damage multipliers, status effects, skill handler patterns, VFX mapping | Adding any new skill handler |
| 04 | `04_Monsters_EnemyAI.md` | 4 | 509 monster templates, AI state machine, aggro/assist, mode flags, monster skills, MVP mechanics, spawn management | Monster AI changes, MVP implementation, spawn config |
| 05 | `05_Items_Equipment_Cards.md` | 5 | Item DB schema, equipment slots, refining, card compound, card effects, weight system, consumable effects | Any item/equipment work, refining, cards |
| 06 | `06_World_Maps_Zones.md` | 3 | Zone definitions, warp network, zone flags, spawn configs, Kafra locations, dungeon connections | Creating new zones, warp portal placement |
| 07 | `07_NPCs_Quests_Shops.md` | 8, 9 | NPC dialogue engine, quest system, job change quests, access quests, shop inventories | NPC dialogue work, quest creation, shop changes |
| 08 | `08_PvP_Guild_WoE.md` | 7 | PvP rules, arena mechanics, guild system, WoE schedule, castle maps, Emperium, guardians | PvP implementation, guild system, WoE |
| 09 | `09_UI_UX_System.md` | All | All widgets (Slate + UMG), subsystem inventory, Z-order map, hotkeys, drag-drop, tooltips | Creating any new UI panel or widget |
| 10 | `10_Art_Animation_VFX_Pipeline.md` | 11 | Character models, monster pipeline, environment art, equipment meshes, VFX catalog, animations | Art asset creation, VFX work, animation |
| 11 | `11_Multiplayer_Networking.md` | 0, 13 | Socket.io event catalog, position sync, interest management, bandwidth optimization, server tick | Network changes, optimization, adding socket events |
| 12 | `12_Pets_Homunculus_Companions.md` | 10 | Pet taming/feeding/intimacy, Homunculus AI, Falcon, Peco mount, Cart | Companion system implementation |
| 13 | `13_Economy_Trading_Vending.md` | 9 | Vending, trade protocol, buying store, storage, zeny flow, anti-dupe | Economy feature implementation |
| 14 | `14_Audio_Music_SFX.md` | 12 | BGM per zone, SFX catalog, 3D spatialization, volume controls, audio assets | Audio system implementation |

### Additional Project Documentation

| Document | Path | Use When |
|----------|------|----------|
| Master Build Plan | `RagnaCloneDocs/00_Master_Build_Plan.md` | Understanding full project scope, phase details, dependency graph |
| Global Rules | `docsNew/00_Global_Rules/Global_Rules.md` | Design standards, coding rules (always follow) |
| Project Overview | `docsNew/00_Project_Overview.md` | Full system inventory, tech stack, feature list |
| Zone Setup Guide | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` | Creating new UE5 levels, Level Blueprint setup |
| VFX Asset Reference | `docsNew/05_Development/VFX_Asset_Reference.md` | Finding VFX assets for skills (1,574 cataloged) |
| VFX Implementation Plan | `docsNew/05_Development/Skill_VFX_Implementation_Plan.md` | VFX architecture, per-skill specs |

---

## 7. Cross-Cutting Rules

These rules apply to EVERY phase. Violation of any rule is a blocker.

### 7.1 Multiplayer Testing Protocol

**Every feature MUST be tested with 2+ PIE instances before completion.**

| Test Type | Frequency |
|-----------|-----------|
| 2-player basic functionality | Every new feature |
| Cross-zone interaction | Every zone-related change |
| Concurrent operations | Every shared-state feature (inventory, trade, combat) |
| Combat synchronization | Every combat change |

### 7.2 Server-Authoritative Validation

| Data | Authority | Client Role |
|------|-----------|-------------|
| Position | Server validates speed | Client sends requests, displays corrections |
| Damage | Server calculates | Client displays results only |
| Inventory | Server tracks | Client requests changes, server confirms |
| Stats | Server calculates | Client displays server-provided values |
| Skills | Server validates | Client sends requests, server checks cooldowns/SP/range |
| Trading | Server mediates | Atomic transactions, item ownership validated |

### 7.3 Socket Event Error Pattern

Every server socket handler follows this pattern:
```javascript
socket.on('event:name', async (data) => {
    try {
        if (!data || !data.requiredField) {
            socket.emit('event:error', { message: 'Invalid input' });
            return;
        }
        // Process...
        socket.emit('event:result', { success: true, ... });
    } catch (err) {
        logger.error(`[event:name] ${err.message}`, { stack: err.stack });
        socket.emit('event:error', { message: 'Server error' });
    }
});
```

### 7.4 C++ Null Safety

Every subsystem that processes socket events:
```cpp
void UMySubsystem::OnEventReceived(const TSharedPtr<FJsonObject>& Data)
{
    if (!Data.IsValid()) { UE_LOG(LogTemp, Warning, TEXT("Null data")); return; }
    // Process...
}
```

### 7.5 Viewport Access (Multiplayer-Safe)

**NEVER** use `GEngine->GameViewport`. Always use `World->GetGameViewport()` from the owning subsystem/world context.

### 7.6 Database Migrations

| Rule | Description |
|------|-------------|
| Forward-only | Never modify existing migration files |
| Naming | `add_<feature>.sql` or `alter_<table>_<change>.sql` |
| Idempotent | Use `IF NOT EXISTS`, `ON CONFLICT DO NOTHING` |
| Rollback | Comment block at top with rollback SQL |

### 7.7 Git Workflow

| Practice | Description |
|----------|-------------|
| Branch per feature | `feature/<phase>-<name>` |
| Commit prefixes | `feat:`, `fix:`, `refactor:`, `docs:`, `test:` |
| No binary commits | `.uasset` not tracked |
| Version tags | Tag after phase completion (e.g., `v0.1-phase0`) |

### 7.8 Documentation Updates

After every code change:
1. Update relevant `docsNew/` file
2. Update `MEMORY.md` if new architectural knowledge
3. Update socket event reference if new events added
4. Update database docs if schema changed

---

## Summary Statistics

| Metric | Count |
|--------|-------|
| Total phases | 14 (0-13) |
| Total checklist tasks | ~350+ |
| Tasks already DONE | ~80+ (see Section 2) |
| Tasks remaining | ~270+ |
| Estimated total duration (solo) | 60-90 weeks |
| Companion documents | 14 spec docs + 6 dev docs |
| Milestone checkpoints | 6 (Alpha 0.1/0.2/0.3, Beta 0.5/0.8, Release 1.0) |

---

## Research Sources

Architecture decisions informed by:
- [Solo UE5 MMO Development Guide (2026)](https://medium.com/@Jamesroha/solo-ue5-mmo-development-guide-f70b9c46d8ac) -- server-authoritative patterns, GameInstance persistence
- [Building a Small-Scale MMO in UE5 (2026)](https://medium.com/@Jamesroha/building-a-small-scale-mmo-in-unreal-engine-5-the-solo-developers-practical-guide-e7c7ab17eaae) -- solo developer practical guide
- [UE5 Programming Subsystems Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine) -- subsystem lifecycle, scope selection
- [Slate UI Architecture (UE5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-slate-ui-architecture-in-unreal-engine) -- Slate framework principles
- [UE5 Subsystem Patterns](https://tech.flying-rat.studio/post/ue-subsystems.html) -- WorldSubsystem vs GameInstanceSubsystem patterns
- [Game Development Patterns with UE5](https://www.packtpub.com/en-us/product/game-development-patterns-with-unreal-engine-5-9781803243252) -- Component pattern, Event Observer pattern
- [Unreal-style Singletons with Subsystems](https://unreal-garden.com/tutorials/subsystem-singleton/) -- subsystem lifecycle management
- [Socket.IO Client for Unreal Engine](https://github.com/getnamo/SocketIOClient-Unreal) -- real-time bidirectional communication
- [OmniMesh MMO Networking](https://starvault.se/omnimesh-mmo-networking-unreal-engine-5/) -- LOD, tick, and packet control for scale
- [Tom Looman UE5 C++ Guide](https://tomlooman.com/unreal-engine-cpp-guide/) -- comprehensive C++ development patterns

---

**This document is the single source of truth for the Sabri_MMO implementation plan. Work through it top-to-bottom. Every task references the companion document that covers its details. Every milestone has testable criteria. No phase should be started until its dependencies are complete.**

**Last Updated**: 2026-03-08
**Status**: Active
**Next Task**: Phase 0, Task 0.1.1 -- Extract shared utilities
