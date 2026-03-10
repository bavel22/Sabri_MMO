# 00 -- Master Build Plan

## Ragnarok Online Classic 3D Replica -- Sabri_MMO

**Document Version**: 1.0
**Date**: 2026-03-08
**Engine**: Unreal Engine 5.7 (C++ + Blueprints)
**Server**: Node.js 18+ / Express / Socket.io 4.8
**Database**: PostgreSQL 15.4 + Redis 7.2
**Architecture**: Server-authoritative (all game logic validated server-side)

---

## Table of Contents

1. [Project Status Assessment](#1-project-status-assessment)
2. [Phase 0: Foundation Hardening](#2-phase-0-foundation-hardening)
3. [Phase 1: Core Combat Completion](#3-phase-1-core-combat-completion)
4. [Phase 2: Class System](#4-phase-2-class-system)
5. [Phase 3: World Expansion](#5-phase-3-world-expansion)
6. [Phase 4: Monster System Expansion](#6-phase-4-monster-system-expansion)
7. [Phase 5: Item and Equipment Deep Dive](#7-phase-5-item-and-equipment-deep-dive)
8. [Phase 6: Social Systems](#8-phase-6-social-systems)
9. [Phase 7: PvP and War of Emperium](#9-phase-7-pvp-and-war-of-emperium)
10. [Phase 8: NPC and Quest System](#10-phase-8-npc-and-quest-system)
11. [Phase 9: Economy](#11-phase-9-economy)
12. [Phase 10: Companion Systems](#12-phase-10-companion-systems)
13. [Phase 11: Art and Polish](#13-phase-11-art-and-polish)
14. [Phase 12: Audio](#14-phase-12-audio)
15. [Phase 13: Optimization and Launch Prep](#15-phase-13-optimization-and-launch-prep)
16. [Cross-Cutting Concerns](#16-cross-cutting-concerns)
17. [Companion Document Index](#17-companion-document-index)

---

## 1. Project Status Assessment

### 1.1 What Is Already Built

The project has significant foundational infrastructure in place. The following is a detailed inventory of completed systems based on the actual codebase.

#### Authentication and Characters (COMPLETE)
- JWT-based login/register via REST API (`POST /api/auth/login`, `POST /api/auth/register`)
- Character CRUD: create, list, select, soft-delete with password confirmation
- Server list endpoint (`GET /api/servers`) with population tracking
- Character customization: hair style (1-19), hair color (0-8), gender
- 9 character slots per account, globally unique names (case-insensitive)
- JWT validation on `player:join` socket event (character ownership check)
- `FCharacterData` C++ struct (30+ fields), `FServerInfo` struct
- `UMMOGameInstance` persists auth state, server selection, remembered username
- Configurable server URL (not hardcoded)

#### Login Flow UI (COMPLETE -- Pure C++ Slate)
- `ULoginFlowSubsystem` -- state machine: Login, ServerSelect, CharacterSelect, CharacterCreate, EnteringWorld
- `SLoginWidget` -- username/password, remember me, error display, Enter/Tab shortcuts
- `SServerSelectWidget` -- scrollable server list, population/status, selection highlighting
- `SCharacterSelectWidget` -- 3x3 card grid, detail panel (HP/SP bars, 6 stats, EXP), delete confirmation
- `SCharacterCreateWidget` -- name field, gender toggle, hair style/color pickers
- `SLoadingOverlayWidget` -- dimmed fullscreen overlay with progress bar

#### Multiplayer Networking (COMPLETE)
- Socket.io bidirectional communication
- `player:join` / `player:position` / `player:moved` / `player:left` events
- Redis position caching with 5-minute TTL
- Remote player spawning, interpolation (`UOtherCharacterMovementComponent`), and name tags
- Zone-scoped broadcasting (`broadcastToZone()` / `broadcastToZoneExcept()`)
- Tested with 5+ concurrent players

#### Combat System (FUNCTIONAL -- needs expansion)
- Server-authoritative auto-attack loop (50ms tick)
- ASPD-based attack intervals (0-195 scale)
- Player-vs-Enemy and Player-vs-Player targeting
- Range checking with tolerance padding
- Death/respawn cycle with HP restore and teleport
- Kill messages broadcast in COMBAT chat channel
- Complete `ro_damage_formulas.js`:
  - Physical damage: StatusATK, WeaponATK, PassiveATK, size penalty, element modifier, DEF reduction, critical hits, perfect dodge, hit/miss, damage variance
  - Magical damage: MATK, element modifier, MDEF reduction
  - Element effectiveness table (10x10x4 -- all 10 elements, 4 levels each)
  - Size penalty table (17 weapon types x 3 sizes)
  - HIT/FLEE calculation with multi-attacker FLEE penalty
  - Critical rate calculation with LUK-based shield

#### Stat System (COMPLETE)
- 6 base stats: STR, AGI, VIT, INT, DEX, LUK
- Derived stats: statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, ASPD, maxHP, maxSP
- Stat point allocation with server validation (RO formula: cost increases with stat value)
- Stats loaded from DB on join, saved on disconnect
- RO Classic regen: HP every 6s, SP every 8s, skill regen every 10s

#### Skill System (PARTIALLY COMPLETE)
- **Skill data**: All first-class and second-class skills defined in `ro_skill_data.js` + `ro_skill_data_2nd.js`
  - Novice: 3 skills (Basic Skill, First Aid, Play Dead)
  - Swordsman: 7 skills (Sword Mastery through Endure)
  - Mage: 13 skills (Cold Bolt through Thunderstorm)
  - Archer: 6 skills (Owl's Eye through Arrow Crafting)
  - Acolyte: 13 skills (Heal through Aqua Benedicta)
  - Thief: 6 skills (Double Attack through Detoxify)
  - Merchant: 8 skills (Enlarge Weight Limit through Change Cart)
  - Knight: 10 skills, Crusader: 7 skills, Wizard: 10 skills, Sage: 5 skills
  - Hunter: 8 skills, Bard: 3 skills, Dancer: 3 skills
  - Priest: 8 skills, Monk: 6 skills
  - Assassin: 7 skills, Rogue: 5 skills
  - Blacksmith: 6 skills, Alchemist: 5 skills
- **Skill tree UI**: `SkillTreeSubsystem` + `SSkillTreeWidget` (K toggle)
- **Skill learning**: `skill:learn`, `skill:reset` events with prerequisite validation
- **Implemented skill handlers** (server-side logic complete, VFX working):
  - Heal (400), First Aid (2), Provoke (104)
  - Cold Bolt (200), Fire Bolt (201), Lightning Bolt (202) -- Pattern A (bolt from sky)
  - Fire Ball (207) -- Pattern B (single projectile + splash)
  - Soul Strike (210) -- Pattern C (multi-hit projectile)
  - Thunderstorm (212) -- Pattern E (ground AoE rain)
  - Frost Diver (208) -- Pattern D (freeze debuff)
  - Stone Curse (206) -- Pattern D (petrify debuff)
  - Sight (205) -- reveal buff
  - Napalm Beat (203) -- ghost AoE
  - Fire Wall (209) -- ground fire barrier
  - Safety Wall (211) -- ground protection barrier
- **NOT implemented**: Bash, Magnum Break, Endure, Double Strafe, Arrow Shower, Blessing, Increase AGI, Decrease AGI, and all other first/second class active skills
- **Cast bar system**: `CastBarSubsystem` + `SCastBarOverlay` -- world-projected cast bars with skill:cast_start/complete/interrupted events
- **Cast time reduction**: DEX-based formula implemented

#### VFX System (FUNCTIONAL)
- `SkillVFXSubsystem` + `SkillVFXData` (config in .cpp, not header -- Live Coding compatible)
- `CastingCircleActor` for ground-targeted skill indicators
- 5 VFX behavior patterns:
  - A (Bolt): N bolts from sky, staggered
  - B (AoE Projectile): single projectile + impact explosion
  - C (Multi-Hit Projectile): N projectiles player-to-enemy
  - D (Persistent Buff): spawned on buff_applied, destroyed on buff_removed
  - E (Ground AoE Rain): N strikes at random positions
- Cascade loop timer for non-looping PSC re-activation

#### Inventory and Equipment (COMPLETE)
- 148 total items: 22 original + 126 RO drops
  - 28 consumables, 58 etc items, 20 weapons, 14 armor, 23 monster cards
- PostgreSQL `items` (definitions) + `character_inventory` (per-character)
- Socket.io events: load, use consumable, equip/unequip, drop/discard, move
- Equipped weapon modifies ATK, range, and ASPD
- Stackable items with max stack limits
- Drag-and-drop between inventory and equipment slots
- Dual accessory support (accessory_1 / accessory_2)
- Weight system tracked but not enforced
- `InventorySubsystem` + `SInventoryWidget` (F6)
- `EquipmentSubsystem` + `SEquipmentWidget` (F7)

#### Hotbar System (COMPLETE)
- 4 rows x 9 slots, F5 cycle visibility
- Server persistence (`character_hotbar` table with `row_index`)
- Items and skills can be placed on hotbar
- Keybind system (default: Row 0 = 1-9, Row 1 = Alt+1-9)
- `HotbarSubsystem` + `SHotbarRowWidget` + `SHotbarKeybindWidget`

#### Enemy/NPC System (FUNCTIONAL)
- 509 RO monster templates from rAthena pre-renewal database
- 46 spawn points active across 3 zones (zones 4-9 disabled)
- Full AI state machine: IDLE, CHASE, ATTACK, DEAD (200ms tick)
- AI code mapping: 1,004 monster ID to rAthena AI type codes
- 18 boolean mode flags parsed from hex bitmasks
- Aggro system: `setEnemyAggro()` called from all damage paths
- Assist trigger: same-type mobs within 550 UE units join combat
- Hit stun (`damageMotion` ms of inaction)
- Boss protocol: knockback/status immune, detector flags
- Move speed: `(50 / walkSpeed) * 1000` UE units/sec
- Drop tables with chance-based loot rolling
- 126 RO drop items integrated with inventory

#### Zone/Map System (FUNCTIONAL -- 4 zones)
- Zone registry: `ro_zone_data.js` (`ZONE_REGISTRY` object)
- 4 zones implemented:
  - `prontera` -- Capital town (no enemies)
  - `prontera_south` -- Starter field (zone 1-2 monsters, 30 spawns)
  - `prontera_north` -- Higher field (zone 3 monsters, 16 spawns)
  - `prt_dungeon_01` -- Prontera Culvert dungeon (10 spawns)
- `ZoneTransitionSubsystem` -- manages zone:change/error/teleport, loading overlay
- `WarpPortal` actors for zone-to-zone transitions
- `KafraSubsystem` + `SKafraWidget` + `KafraNPC` -- save point, teleport service
- Level Blueprint spawns and possesses `BP_MMOCharacter`
- Position persistence: disconnect + 60s periodic + 5s Level Blueprint save
- Login loads correct map directly from REST character data

#### Shop System (FUNCTIONAL)
- `ShopSubsystem` + `SShopWidget` + `ShopNPC`
- `shop:open`, `shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch` events
- Batch buy/sell with Discount/Overcharge skill integration

#### Job Change System (FUNCTIONAL)
- `job:change` socket event with full validation
- Novice to first class: requires Job Level 10
- First to second class: requires Job Level 40+
- Valid upgrade paths defined in `SECOND_CLASS_UPGRADES`
- Job level/exp reset on change, skill points kept
- System announcement broadcast

#### EXP and Leveling (COMPLETE)
- Base EXP table (levels 1-99)
- Novice Job EXP table (1-10)
- First Class Job EXP table (1-50)
- Second Class Job EXP table (1-50)
- Job class config with max levels and display names
- Skill points awarded per job level

#### UI System -- 14 C++ Slate Subsystems/Widgets
| Subsystem | Widget | Hotkey | Z-Order |
|-----------|--------|--------|---------|
| `LoginFlowSubsystem` | 5 login widgets | -- | 5/50 |
| `BasicInfoSubsystem` | `SBasicInfoWidget` | -- | 10 |
| `CombatStatsSubsystem` | `SCombatStatsWidget` | F8 | 12 |
| `InventorySubsystem` | `SInventoryWidget` | F6 | 14 |
| `EquipmentSubsystem` | `SEquipmentWidget` | F7 | 15 |
| `HotbarSubsystem` | `SHotbarRowWidget` x4 | F5 | 16/30 |
| `SkillTreeSubsystem` | `SSkillTreeWidget` | K | 20 |
| `DamageNumberSubsystem` | `SDamageNumberOverlay` | -- | 20 |
| `CastBarSubsystem` | `SCastBarOverlay` | -- | 25 |
| `WorldHealthBarSubsystem` | `SWorldHealthBarOverlay` | -- | 8 |
| `ShopSubsystem` | `SShopWidget` | -- | 18 |
| `KafraSubsystem` | `SKafraWidget` | -- | 19 |
| `ZoneTransitionSubsystem` | (loading overlay) | -- | 50 |

#### Additional Blueprint Widgets (UMG)
- `WBP_GameHUD`, `WBP_PlayerNameTag`, `WBP_ChatWidget`, `WBP_StatAllocation`
- `WBP_TargetHealthBar`, `WBP_InventoryWindow`, `WBP_ContextMenu`
- `WBP_DragVisual`, `WBP_ItemTooltip`, `WBP_DeathOverlay`, `WBP_LootPopup`

#### Testing Infrastructure
- C++ test classes: `SabriMMOCombatTests`, `SabriMMODatabaseTests`, `SabriMMOIntegrationTests`, `SabriMMONetworkTests`, `SabriMMOServerTests`, `SabriMMOUITests`

### 1.2 What Is Partially Built

| System | Status | Gaps |
|--------|--------|------|
| **Skill handlers** | 15/86+ skills have server logic | Need handlers for remaining Swordsman, Archer, Acolyte, Thief, Merchant, and ALL second-class skills |
| **VFX per skill** | ~10 skills have VFX | Need VFX patterns for 70+ remaining skills |
| **Status effects** | Freeze (Frost Diver) and Petrify (Stone Curse) implemented | Missing: Poison, Stun, Sleep, Curse, Silence, Blind, Bleeding, Coma, Provoke ATK/DEF change is implemented |
| **Damage formulas** | Physical + Magical complete | Size penalty not fully wired into auto-attack flow; race/element modifiers present in formula module but not always passed by handlers |
| **ASPD formula** | Generic sqrt-based | Need per-class per-weapon ASPD tables from RO data |
| **Weight system** | Weight column exists on items | Not enforced (no "overweight" penalty, no movement penalty) |
| **Card system** | 23 cards in DB with stat descriptions | No compound (insert into equipment) or effect application logic |
| **Chat system** | Global chat working | Missing ZONE, PARTY, GUILD, TELL channels |
| **Monster skills** | None | Monsters only auto-attack; need monster skill system |
| **Class-specific features** | Job change works | No class-specific stat bonuses, weapon restrictions, or ASPD tables |

### 1.3 What Is Not Started

| System | Notes |
|--------|-------|
| **Party system** | No server events, no UI, no shared EXP |
| **Guild system** | No DB tables, no events, no UI |
| **PvP system** | Basic PvP targeting exists but no arenas, rules, or modes |
| **War of Emperium** | Not started |
| **Quest system** | No quest engine, no quest DB, no quest UI |
| **NPC dialogue engine** | Shop NPCs exist but no dialogue tree system |
| **Trading system** | No player-to-player trade |
| **Vending system** | Skill data exists but no player shop functionality |
| **Buying Store** | Not started |
| **Storage system** | Kafra NPC exists but no storage service |
| **Pet system** | Not started |
| **Homunculus** | Not started |
| **Falcon/Peco/Cart** | Not started (companion systems) |
| **Equipment refining** | Not started |
| **Minimap** | Not started |
| **World map** | Not started |
| **Character model pipeline** | Using placeholder UE5 mannequin |
| **Monster models** | Using placeholder shapes |
| **Environment art** | Basic landscape only |
| **Audio system** | No BGM, no SFX |
| **Transcendent classes** | No rebirth system |
| **Extended/third classes** | Not in scope for initial release |

### 1.4 Technical Debt

| Issue | Severity | Location | Description |
|-------|----------|----------|-------------|
| **Monolithic server** | HIGH | `server/src/index.js` (8,421 lines) | Single file contains all server logic. Should be split into modules (combat, skills, inventory, zones, etc.) |
| **Dead C++ code** | LOW | `ASabriMMOPlayerController` | Class exists but is not used by the game (Blueprint `PC_MMOPlayerController` is used instead) |
| **Variant directories** | LOW | `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` | 76 variant files from experimental prototypes, not used by the MMO |
| **Mixed widget systems** | MEDIUM | UI layer | Some UMG widgets + some Slate widgets. Should standardize on Slate for new features |
| **Weight not enforced** | MEDIUM | Server | Weight column exists on items but overweight penalties and movement restrictions not implemented |
| **Generic ASPD formula** | MEDIUM | `ro_damage_formulas.js` | Uses sqrt-based generic formula instead of RO-accurate per-class per-weapon tables |
| **No input validation on positions** | MEDIUM | `player:position` handler | Server stores positions from client without speed/teleport validation |
| **HP/SP not class-accurate** | LOW | `calculateDerivedStats()` | Uses simplified class-agnostic HP/SP formula instead of RO per-class tables |
| **Skill handler duplication** | MEDIUM | `index.js` lines 3235-4815 | Each skill has its own large handler block; should extract common patterns |

---

## 2. Phase 0: Foundation Hardening

**Goal**: Stabilize the existing codebase, resolve technical debt, establish performance baselines, and create the infrastructure needed for rapid feature development.

**Estimated Tasks**: 25-30
**Dependencies**: None (this phase enables all others)
**Duration Estimate**: 2-3 weeks

### 2.1 Server Modularization

Split `server/src/index.js` (8,421 lines) into focused modules:

| Module | Responsibility | Estimated Lines |
|--------|---------------|-----------------|
| `server/src/index.js` | Express setup, Socket.io init, module wiring | ~300 |
| `server/src/modules/auth.js` | REST auth endpoints, JWT middleware | ~200 |
| `server/src/modules/characters.js` | Character CRUD REST endpoints | ~300 |
| `server/src/modules/combat.js` | Auto-attack loop, damage calculation wrappers | ~500 |
| `server/src/modules/skills.js` | `skill:use` mega-handler, cast system | ~1500 |
| `server/src/modules/inventory.js` | Inventory/equip/drop/use events | ~600 |
| `server/src/modules/enemies.js` | Enemy AI loop, spawn management | ~800 |
| `server/src/modules/zones.js` | Zone transitions, warp events | ~300 |
| `server/src/modules/chat.js` | Chat message routing | ~100 |
| `server/src/modules/shops.js` | NPC shop buy/sell events | ~400 |
| `server/src/modules/stats.js` | Stat allocation, derived stat calculation | ~300 |
| `server/src/modules/players.js` | Player join/position/disconnect | ~500 |
| `server/src/modules/buffs.js` | Buff/debuff application, duration tracking, expiry | ~400 |
| `server/src/shared/logger.js` | Structured logger (already exists, extract) | ~50 |
| `server/src/shared/constants.js` | All game constants | ~100 |
| `server/src/shared/db.js` | PostgreSQL pool, Redis client | ~50 |
| `server/src/shared/utils.js` | Shared utilities (findPlayer, broadcastToZone, etc.) | ~200 |

**Key files to modify**: `server/src/index.js` (break apart), `server/package.json` (no new deps needed)
**Testing checklist**:
- [ ] All existing socket events still work after modularization
- [ ] Server starts without errors
- [ ] Login/auth flow unchanged
- [ ] Combat still functions
- [ ] Skills still fire correctly
- [ ] Inventory operations unchanged
- [ ] Zone transitions unchanged

### 2.2 Skill Handler Refactoring

Extract common patterns from the 1,500+ line skill handler:

```
extractSkillPattern('bolt', [200, 201, 202]);      // Cold/Fire/Lightning Bolt
extractSkillPattern('splash', [203, 207]);           // Napalm Beat, Fire Ball
extractSkillPattern('multiHit', [210]);              // Soul Strike
extractSkillPattern('groundAoe', [212]);             // Thunderstorm
extractSkillPattern('debuff', [208, 206]);           // Frost Diver, Stone Curse
extractSkillPattern('selfBuff', [205, 106, 302]);    // Sight, Endure, Improve Concentration
extractSkillPattern('heal', [400, 2]);               // Heal, First Aid
```

Each pattern becomes a reusable function that takes skill config and produces the correct behavior. New skills can then be added by specifying config, not writing 100+ line handlers.

### 2.3 Performance Baseline

Establish measurements before adding features:

| Metric | Tool | Target |
|--------|------|--------|
| Server response time (REST) | Artillery.io load test | < 50ms p95 |
| Socket.io event latency | Custom timestamp script | < 100ms round-trip |
| Combat tick accuracy | Server-side timer drift logging | +/- 5ms of 50ms target |
| Client FPS | UE5 stat fps | > 60 FPS with 20 players + 50 enemies |
| Memory usage (server) | `process.memoryUsage()` logging | < 512MB with 100 players |
| DB query time | pg pool instrumentation | < 10ms per query |

**Key files to create**:
- `scripts/load_test.js` -- Artillery/custom load test harness
- `scripts/perf_baseline.js` -- Automated performance measurement

### 2.4 Database Optimization

| Task | Priority | Notes |
|------|----------|-------|
| Add composite indexes | HIGH | `character_inventory(character_id, is_equipped)`, `character_hotbar(character_id, row_index)` |
| Connection pool tuning | MEDIUM | Current default pool size; tune based on load test results |
| Add DB connection retry | MEDIUM | Server should retry on connection failure instead of crashing |
| Audit N+1 queries | HIGH | Check skill:data, inventory:load for unnecessary loops |
| Add query timing logs | LOW | Instrument slow queries |

**Database migrations needed**:
- `database/migrations/add_performance_indexes.sql`

### 2.5 Code Cleanup

| Task | Priority |
|------|----------|
| Remove or archive `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` directories | LOW |
| Document all socket events in a single reference file | MEDIUM |
| Standardize error handling across all socket handlers (consistent error event format) | HIGH |
| Add input validation to `player:position` (speed check, teleport detection) | HIGH |
| Review all `IsValid` null guards in C++ subsystems | MEDIUM |

### 2.6 Definition of Done -- Phase 0

- [ ] Server modularized into 10+ files, all tests pass
- [ ] Skill handlers use shared pattern functions
- [ ] Performance baseline document created with numbers
- [ ] All critical database indexes in place
- [ ] Position validation prevents speed hacking
- [ ] Error handling standardized across all socket events
- [ ] Zero regressions in existing functionality

---

## 3. Phase 1: Core Combat Completion

**Goal**: Implement the complete RO pre-renewal combat system including all first-class skill handlers, the full status effect system, accurate ASPD tables, and proper element/size/race integration.

**Estimated Tasks**: 60-80
**Dependencies**: Phase 0 (modularized skill system makes this tractable)
**Duration Estimate**: 4-6 weeks

### 3.1 ASPD Formula Accuracy

Replace the generic sqrt-based ASPD formula with RO-accurate per-class per-weapon tables.

**RO Pre-Renewal ASPD Formula**:
```
ASPD = 200 - (BaseASPD[class][weaponType] - floor(AGI * 0.25) - floor(DEX * 0.1))
```

Each class has a different base ASPD for each weapon type. This requires a lookup table.

**Key files to modify**: `server/src/ro_damage_formulas.js`
**Key files to create**: `server/src/ro_aspd_tables.js`

**Data needed (per-class per-weapon-type)**:
| Class | Bare | Dagger | 1H Sword | 2H Sword | Spear | Axe | Mace | Rod | Bow | Katar | Book | Knuckle |
|-------|------|--------|----------|----------|-------|-----|------|-----|-----|-------|------|---------|
| Novice | 200 | 200 | 200 | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Swordsman | 200 | 196 | 195 | 195 | 195 | 196 | 195 | -- | -- | -- | -- | -- |
| Mage | 200 | 196 | -- | -- | -- | -- | -- | 195 | -- | -- | -- | -- |
| ... | ... | ... | ... | ... | ... | ... | ... | ... | ... | ... | ... | ... |

### 3.2 Status Effect System

Implement the complete RO status effect framework. Each status has: application logic, duration tracking, tick effects, removal conditions, visual indicator, and icon.

| Status | ID | Effect | Duration | Visual |
|--------|----|--------|----------|--------|
| **Poison** | 1 | -HP/tick (every 3s), reduces HP regen by 50% | Skill dependent | Green tint + drip particles |
| **Stun** | 2 | Cannot move/attack/cast | 2-5s (skill dependent) | Stars above head |
| **Freeze** | 3 | Cannot act, Water 1 property, +50% fire dmg | Skill dependent | DONE (Frost Diver) |
| **Petrify** | 4 | Phase 1: slowing. Phase 2: cannot act, +50% fire dmg, Earth 1 property | 20s | DONE (Stone Curse) |
| **Sleep** | 5 | Cannot act, wake on damage | Skill dependent | Zzz particles |
| **Curse** | 6 | LUK = 0, -25% move speed | 30s | Ghost sprite |
| **Silence** | 7 | Cannot use skills | 30s | "..." speech bubble |
| **Blind** | 8 | -25 HIT, -25% FLEE, reduced vision | 30s | Dark screen overlay |
| **Bleeding** | 9 | -HP/tick (every 5s), no natural regen | 30s | Red drip particles |
| **Coma** | 10 | HP = 1, SP = 1 (instant) | Instant | Flash effect |

**Server implementation** (`server/src/modules/buffs.js`):
```javascript
// Status effect registry
const STATUS_EFFECTS = {
    POISON: { id: 1, name: 'poison', stackable: false, ... },
    STUN: { id: 2, name: 'stun', stackable: false, ... },
    // ...
};

// Functions needed:
applyStatusEffect(target, statusId, duration, sourceId)
removeStatusEffect(target, statusId)
tickStatusEffects(target)          // Called from combat tick
hasStatusEffect(target, statusId)
getActiveEffects(target)
```

**Socket.io events to add**:
| Event | Direction | Payload |
|-------|-----------|---------|
| `status:applied` | S->C | `{ targetId, targetType, statusId, statusName, duration, sourceId }` |
| `status:removed` | S->C | `{ targetId, targetType, statusId }` |
| `status:tick` | S->C | `{ targetId, targetType, statusId, damage }` (for DoT effects) |

**Client C++ changes**: `StatusEffectSubsystem` (new UWorldSubsystem) for tracking and displaying active status icons.

### 3.3 Implement Remaining First-Class Skill Handlers

Each skill needs: server handler, damage/effect logic, socket events, VFX config, and integration testing.

#### Swordsman Skills (5 remaining)

| Skill | ID | Pattern | Priority | Notes |
|-------|----|---------|----------|-------|
| Bash | 103 | Physical single-target + multiplier | HIGH | Core skill, 130-430% damage |
| Magnum Break | 105 | Physical AoE + fire element | HIGH | Pushback + fire endow |
| Endure | 106 | Self-buff (MDEF + flinch immunity) | MEDIUM | Duration 10-40s |
| Sword Mastery | 100 | Passive (ATK bonus) | HIGH | Already in stat calc? Verify |
| 2H Sword Mastery | 101 | Passive (ATK bonus) | MEDIUM | Weapon-type conditional |

#### Archer Skills (3 remaining active)

| Skill | ID | Pattern | Priority | Notes |
|-------|----|---------|----------|-------|
| Double Strafe | 303 | 2-hit ranged attack | HIGH | Core archer skill |
| Arrow Shower | 304 | Ground AoE arrows | HIGH | 5x5 splash |
| Improve Concentration | 302 | Self-buff (AGI/DEX) + reveal hidden | MEDIUM | Party-wide effect later |

#### Acolyte Skills (8 remaining active)

| Skill | ID | Pattern | Priority | Notes |
|-------|----|---------|----------|-------|
| Blessing | 402 | Single buff (STR/DEX/INT) | HIGH | Core support skill |
| Increase AGI | 403 | Single buff (AGI) | HIGH | Core support skill |
| Decrease AGI | 404 | Single debuff (AGI) | MEDIUM | |
| Cure | 405 | Remove status effects | MEDIUM | Depends on status system |
| Angelus | 406 | Party buff (VIT DEF) | MEDIUM | Party system later |
| Signum Crucis | 407 | AoE debuff vs Undead/Demon | LOW | Niche use |
| Ruwach | 408 | Reveal hidden | LOW | |
| Teleport | 409 | Random teleport / save point | MEDIUM | Zone flag checks needed |
| Warp Portal | 410 | Ground portal | LOW | Complex -- needs portal entity system |
| Pneuma | 411 | Block ranged attacks zone | LOW | Complex ground effect |

#### Thief Skills (3 remaining active)

| Skill | ID | Pattern | Priority | Notes |
|-------|----|---------|----------|-------|
| Envenom | 504 | Physical + poison chance | HIGH | Core thief skill |
| Steal | 502 | Attempt steal from monster | MEDIUM | Needs steal success formula |
| Hiding | 503 | Toggle invisibility | MEDIUM | Needs hidden state system |
| Detoxify | 505 | Cure poison | MEDIUM | Depends on status system |

#### Merchant Skills (2 remaining active)

| Skill | ID | Pattern | Priority | Notes |
|-------|----|---------|----------|-------|
| Mammonite | 603 | Physical + zeny cost | HIGH | Core merchant attack |
| Vending | 605 | Open player shop | LOW | Phase 9 (Economy) |

### 3.4 Element System Integration

The 10x10x4 element table is already implemented in `ro_damage_formulas.js`. What remains:

| Task | Priority |
|------|----------|
| Wire element data from monster templates into damage calls | HIGH |
| Support weapon element endow skills (later phases) | MEDIUM |
| Add armor element to player equipment | MEDIUM |
| Display element weakness/resistance in target info | LOW |
| Elemental arrows for archers | MEDIUM |

### 3.5 Size and Race Modifier Integration

Size penalties exist in `ro_damage_formulas.js` but need full wiring:

| Task | Priority |
|------|----------|
| Pass `weaponType` from equipped weapon to physical damage calls | HIGH |
| Pass `targetSize` from monster template to damage calls | HIGH |
| Pass `targetRace` from monster template to damage calls | HIGH |
| Card race/element/size bonus calculation | MEDIUM (Phase 5) |

### 3.6 Critical Hit System Refinement

Current state: critical formula exists. Remaining work:

| Task | Priority |
|------|----------|
| Critical animation/sound on client | MEDIUM |
| Double damage on critical (currently just max ATK) | HIGH |
| Katar double critical bonus | MEDIUM |
| Lucky Dodge display | LOW |

### 3.7 Definition of Done -- Phase 1

- [ ] All 6 first-class skill trees have working active skills (server handlers)
- [ ] Status effect system supports all 10 core statuses
- [ ] ASPD uses per-class per-weapon tables
- [ ] Element/size/race modifiers are fully wired into all damage paths
- [ ] Critical hits deal correct damage and show distinct visual
- [ ] All passive skills correctly modify stats
- [ ] Buff/debuff system tracks durations server-side with proper expiry
- [ ] VFX exists for all implemented skills
- [ ] Tested with 2+ players simultaneously

---

## 4. Phase 2: Class System

**Goal**: Complete the job change flow, implement class-specific mechanics, and add all second-class skill handlers.

**Estimated Tasks**: 80-100
**Dependencies**: Phase 1 (first-class skills must work)
**Duration Estimate**: 6-8 weeks

### 4.1 Job Change System Polish

The job change mechanism (`job:change` event) is functional. Remaining work:

| Task | Priority |
|------|----------|
| Job Change NPC actors in Prontera | HIGH |
| Job change quest requirements (Phase 8) | LOW (defer) |
| Job change cutscene/celebration effect | LOW |
| Class costume/visual change on job change | MEDIUM |
| Broadcast class change to all players in zone | DONE |

### 4.2 Class-Specific Stat Bonuses

Each class has inherent bonuses to derived stats that are not from equipment:

| Class | HP Modifier | SP Modifier | Special |
|-------|------------|-------------|---------|
| Swordsman | +30% HP | Normal | Higher natural DEF |
| Mage | Normal | +30% SP | Higher MATK base |
| Archer | Normal | Normal | Longer range base |
| Acolyte | +10% HP | +10% SP | Holy resistance |
| Thief | Normal | Normal | Higher Flee base |
| Merchant | Normal | Normal | Zeny skills |

**Key files to modify**: `server/src/ro_damage_formulas.js` (add class param to `calculateDerivedStats`)

### 4.3 Class-Specific Weapon Restrictions

| Class | Allowed Weapons |
|-------|----------------|
| Novice | Dagger only |
| Swordsman | Dagger, 1H Sword, 2H Sword |
| Mage | Rod, Staff |
| Archer | Bow only |
| Acolyte | Mace, Rod |
| Thief | Dagger, Katar (Assassin only) |
| Merchant | Dagger, 1H Sword, Axe, Mace |

**Key files to modify**: `server/src/modules/inventory.js` (equip validation)

### 4.4 Second-Class Skill Implementation

All second-class skill data is already defined in `ro_skill_data_2nd.js`. Need server handlers for each.

**Priority order** (based on gameplay impact):

#### Tier 1 -- Core combat classes (implement first)
1. **Knight** (10 skills): Pierce, Spear Stab, Brandish Spear, Bowling Bash, Two-Hand Quicken, Auto Counter, Spear Boomerang, Riding, Cavalry Mastery
2. **Wizard** (10 skills): Jupitel Thunder, Storm Gust, Meteor Storm, Lord of Vermilion, Earth Spike, Heaven's Drive, Water Ball, Ice Wall, Quagmire, Sight Rasher
3. **Priest** (8 skills): Sanctuary, Kyrie Eleison, Magnificat, Gloria, Resurrection, Magnus Exorcismus, Turn Undead, Lex Aeterna
4. **Assassin** (7 skills): Sonic Blow, Grimtooth, Cloaking, Poison React, Venom Dust, Sonic Acceleration, Katar Mastery
5. **Hunter** (8 skills): Blitz Beat, Steel Crow, Detect, Ankle Snare, Land Mine, Remove Trap, Shockwave Trap, Claymore Trap

#### Tier 2 -- Support and utility classes
6. **Crusader** (7 skills): Holy Cross, Grand Cross, Guard, Shield Charge, Shield Boomerang, Devotion, Faith
7. **Monk** (6 skills): Iron Fists, Summon Spirit Sphere, Occult Impaction, Investigate, Finger Offensive, Asura Strike
8. **Blacksmith** (6 skills): Adrenaline Rush, Weapon Perfection, Power Thrust, Maximize Power, Weaponry Research, Skin Tempering

#### Tier 3 -- Specialized classes
9. **Sage** (5 skills): Study, Cast Cancel, Hindsight, Dispell, Magic Rod
10. **Rogue** (5 skills): Snatcher, Back Stab, Tunnel Drive, Raid, Intimidate
11. **Alchemist** (5 skills): Pharmacy, Acid Terror, Demonstration, Summon Flora, Axe Mastery
12. **Bard** (3 skills): Music Lessons, A Poem of Bragi, Assassin Cross of Sunset
13. **Dancer** (3 skills): Dance Lessons, Service For You, Humming

### 4.5 New Skill Patterns Needed

| Pattern | Skills | Implementation |
|---------|--------|----------------|
| **Trap** | Ankle Snare, Land Mine, Claymore, Shockwave | Ground-placed entity with trigger radius, duration, and effect |
| **Mounted Combat** | Brandish Spear, Cavalry Mastery | Mount state on player, ASPD/speed modifiers |
| **Spirit Sphere** | Summon Spirit Sphere, Finger Offensive, Asura Strike | Counter resource on Monk, consumed by skills |
| **Resurrection** | Resurrection | Target dead player, restore HP%, clear death state |
| **Performance** | Bragi, Assassin Cross, Service For You, Humming | AoE buff centered on caster while channeling |
| **Counter** | Auto Counter, Poison React | Trigger on being hit, consume buff charge |

### 4.6 Transcendent Classes (Deferred)

The rebirth system (base level 99 -> high novice level 1 -> transcendent class) is not in scope for initial launch but the architecture should support it:

- `CLASS_PROGRESSION` in `ro_skill_data.js` already has chains defined
- Need: rebirth NPC, stat reset, transcendent class skill data, trans-exclusive skills
- Defer to post-launch milestone

### 4.7 Database Migrations

```sql
-- Already exists: add_class_skill_system.sql (job_class, job_level, job_exp, skill_points, learned_skills)
-- New migrations needed:
ALTER TABLE characters ADD COLUMN IF NOT EXISTS weapon_restrictions JSONB DEFAULT NULL;
-- (Or handle entirely server-side via class config -- preferred approach)
```

### 4.8 Definition of Done -- Phase 2

- [ ] All 6 first-class job changes functional via NPC interaction
- [ ] All second-class job changes functional (first to second)
- [ ] Class-specific HP/SP formulas implemented
- [ ] Weapon restrictions enforced per class
- [ ] ASPD tables per class per weapon type
- [ ] All Tier 1 second-class skills have working handlers
- [ ] All Tier 2 second-class skills have working handlers
- [ ] Trap system working for Hunter
- [ ] Spirit sphere system working for Monk
- [ ] Performance/song system basic framework for Bard/Dancer
- [ ] Tested with 2+ players, multiple classes

---

## 5. Phase 3: World Expansion

**Goal**: Build out the game world from 4 zones to 30+ zones covering all major RO towns, fields, and dungeons around Prontera, Geffen, Payon, and Morroc.

**Estimated Tasks**: 40-50 (per region)
**Dependencies**: Phase 0 (zone infrastructure), Phase 4 (monsters for each zone)
**Duration Estimate**: 8-12 weeks (ongoing alongside other phases)

### 5.1 Zone Creation Workflow

Each zone requires:
1. UE5 Level: Duplicate existing level (always from `L_PrtSouth` template), rename, modify landscape
2. Level Blueprint: Already in template -- spawns pawn, saves position, handles cleanup
3. Zone Registry Entry: Add to `server/src/ro_zone_data.js` with name, type, flags, spawn, warps, Kafra, enemy spawns
4. Warp Portals: Place `WarpPortal` actors in UE5 with matching warp IDs
5. Kafra NPCs: Place `KafraNPC` actors if town zone
6. Enemy Spawns: Define in zone registry, monsters spawn when zone is active
7. DB Migration: None needed (zone data is code-based)

### 5.2 Prontera Region (PARTIALLY DONE)

| Zone | Type | RO Name | Status | Monster Levels |
|------|------|---------|--------|---------------|
| `prontera` | Town | prontera | DONE | -- |
| `prontera_south` | Field | prt_fild08 | DONE | 1-15 |
| `prontera_north` | Field | prt_fild01 | DONE | 10-20 |
| `prt_dungeon_01` | Dungeon | prt_sewb1 | DONE | 15-25 |
| `prt_dungeon_02` | Dungeon | prt_sewb2 | TODO | 25-40 |
| `prontera_east` | Field | prt_fild02 | TODO | 10-20 |
| `prontera_west` | Field | prt_fild03 | TODO | 15-25 |

### 5.3 Geffen Region

| Zone | Type | RO Name | Status | Monster Levels |
|------|------|---------|--------|---------------|
| `geffen` | Town | geffen | TODO | -- |
| `geffen_field_01` | Field | gef_fild00 | TODO | 20-30 |
| `geffen_field_02` | Field | gef_fild01 | TODO | 25-35 |
| `geffen_tower_01` | Dungeon | gef_tower | TODO | 30-40 |
| `geffen_tower_02` | Dungeon | gef_tower2 | TODO | 35-50 |
| `geffen_tower_03` | Dungeon | gef_tower3 | TODO | 40-55 |
| `geffen_dungeon_01` | Dungeon | gef_dun00 | TODO | 50-65 |
| `geffen_dungeon_02` | Dungeon | gef_dun01 | TODO | 60-75 |

### 5.4 Payon Region

| Zone | Type | RO Name | Status | Monster Levels |
|------|------|---------|--------|---------------|
| `payon` | Town | payon | TODO | -- |
| `payon_field_01` | Field | pay_fild01 | TODO | 15-25 |
| `payon_forest_01` | Field | pay_fild02 | TODO | 20-30 |
| `payon_cave_01` | Dungeon | pay_dun00 | TODO | 25-35 |
| `payon_cave_02` | Dungeon | pay_dun01 | TODO | 30-45 |
| `payon_cave_03` | Dungeon | pay_dun02 | TODO | 40-55 |
| `payon_cave_04` | Dungeon | pay_dun03 | TODO | 55-70 |
| `payon_cave_05` | Dungeon | pay_dun04 | TODO | 65-80 |

### 5.5 Morroc Region

| Zone | Type | RO Name | Status | Monster Levels |
|------|------|---------|--------|---------------|
| `morroc` | Town | morocc | TODO | -- |
| `morroc_field_01` | Field | moc_fild01 | TODO | 15-30 |
| `morroc_field_02` | Field | moc_fild02 | TODO | 20-35 |
| `morroc_pyramids_01` | Dungeon | moc_pryd01 | TODO | 25-40 |
| `morroc_pyramids_02` | Dungeon | moc_pryd02 | TODO | 35-50 |
| `morroc_pyramids_03` | Dungeon | moc_pryd03 | TODO | 45-60 |
| `morroc_pyramids_04` | Dungeon | moc_pryd04 | TODO | 55-70 |
| `morroc_sphinx_01` | Dungeon | moc_sphinx1 | TODO | 50-65 |
| `morroc_sphinx_02` | Dungeon | moc_sphinx2 | TODO | 60-75 |
| `morroc_sphinx_03` | Dungeon | moc_sphinx3 | TODO | 70-85 |
| `morroc_sphinx_04` | Dungeon | moc_sphinx4 | TODO | 75-90 |
| `morroc_sphinx_05` | Dungeon | moc_sphinx5 | TODO | 80-99 |

### 5.6 Additional Towns (Later Phases)

| Town | RO Name | Priority | Reason |
|------|---------|----------|--------|
| Alberta | alberta | MEDIUM | Port town, Merchant guild |
| Izlude | izlude | MEDIUM | Swordsman guild, arena |
| Aldebaran | aldebaran | LOW | Clock Tower dungeon |
| Comodo | comodo | LOW | Beach town, dancers/bards |
| Amatsu | amatsu | LOW | Japanese themed |
| Louyang | louyang | LOW | Chinese themed |
| Einbroch | einbroch | LOW | Industrial city |
| Lighthalzen | lighthalzen | LOW | Biolab dungeon |
| Rachel | rachel | LOW | Temple city |

### 5.7 Warp Portal Network

Create a complete warp network connecting all zones:

```
Prontera ←→ Prontera South ←→ Prontera Dungeon
Prontera ←→ Prontera North
Prontera ←→ Izlude
Prontera ←→ Geffen Fields ←→ Geffen ←→ Geffen Tower/Dungeon
Prontera ←→ Payon Fields ←→ Payon ←→ Payon Cave
Prontera ←→ Morroc Fields ←→ Morroc ←→ Pyramids/Sphinx
```

### 5.8 Kafra Service Expansion

Each town needs a Kafra NPC with:
- Save point (set respawn location)
- Storage (Phase 9)
- Teleport service to other towns (zeny cost varies by distance)
- Cart rental for Merchants (Phase 10)

### 5.9 Definition of Done -- Phase 3

- [ ] 15+ zones playable (4 towns + 11+ fields/dungeons)
- [ ] All warp portals connect correctly
- [ ] Kafra NPCs in all towns with save/teleport
- [ ] Each zone has appropriate monster spawns
- [ ] Zone flags (noteleport, noreturn, nosave) enforced
- [ ] Loading transitions smooth between all zones
- [ ] No zone-specific crashes or stuck states
- [ ] Tested zone transitions with 2+ players

---

## 6. Phase 4: Monster System Expansion

**Goal**: Expand monster variety, implement monster skills, add MVP bosses, and complete the drop table system.

**Estimated Tasks**: 50-60
**Dependencies**: Phase 1 (status effects for monster skills), Phase 3 (zones to spawn in)
**Duration Estimate**: 4-6 weeks

### 6.1 Monster Template Expansion

Current: 509 templates loaded, 46 spawns active. Plan:

| Level Range | Zone Type | Example Monsters | Count |
|-------------|-----------|-----------------|-------|
| 1-10 | Starter fields | Poring, Lunatic, Fabre, Pupa | DONE (15) |
| 10-20 | Fields | Zombie, Skeleton, Spore, Poporing | DONE (16) |
| 20-30 | Fields/Dungeons | Golem, Skel Worker, Goblin, Orc | TODO |
| 30-45 | Mid-level dungeons | Orc Warrior, Mummy, Sandman | TODO |
| 45-60 | High-level fields | Penomena, Anolian, Pest | TODO |
| 60-80 | Deep dungeons | Injustice, Raydric, Dark Lord minions | TODO |
| 80-99 | End-game | High Wizard, Sniper, Assaulter | TODO |

Activate zones 4-9 in `ZONE_REGISTRY` as their corresponding levels are built.

### 6.2 Monster Skill System

Monsters in RO have skills. System needs:

| Component | Description |
|-----------|-------------|
| **Skill database** | `server/src/ro_monster_skills.js` -- per-monster skill list with IDs, cast rates, conditions |
| **Skill execution** | In AI combat tick, check if monster can/should use a skill |
| **Skill conditions** | HP threshold (<50%), target count (>3), random chance per tick |
| **Skill effects** | Same damage/status formulas as player skills |
| **Visual feedback** | `enemy:skill_used` event for client VFX |

**Common monster skills**:
| Skill | Monsters That Use It | Effect |
|-------|---------------------|--------|
| Heal | High Priest type monsters | Self-heal |
| Fireball | Mage-type monsters | AoE fire damage |
| Teleport | Creamy, high-level monsters | Random position |
| Summon | MVPs | Spawn minion monsters |
| Critical Slash | Assassin-type monsters | High-damage melee |
| Stone Curse | Basilisk | Petrify target |
| Frost Diver | Ice-element monsters | Freeze target |

**Socket.io events to add**:
| Event | Direction | Payload |
|-------|-----------|---------|
| `enemy:skill_used` | S->C | `{ enemyId, skillId, skillName, targetId, targetType, damage, statusApplied }` |
| `enemy:cast_start` | S->C | `{ enemyId, skillId, skillName, castTime }` |

### 6.3 MVP Boss System

MVP (Most Valuable Player) bosses are the signature RO end-game PvE content.

| Feature | Description |
|---------|-------------|
| **Unique spawn timers** | Respawn 1-2 hours after death, exact time varies |
| **Spawn announcement** | Server-wide or zone-wide notification |
| **Tombstone** | Visual marker at death location showing killer and time |
| **MVP drops** | Separate from normal drops, only killer gets MVP drop |
| **Boss protocol** | Already implemented: knockback immune, status immune |
| **Phase mechanics** | HP threshold triggers (summon minions, enrage, skill unlock) |

**Initial MVP list** (in level order):

| MVP | Level | Zone | Signature Mechanic |
|-----|-------|------|-------------------|
| Golden Bug | 45 | Sewers B4 | Reflect magic |
| Phreeoni | 53 | Morroc Fields | High FLEE |
| Drake | 60 | Sunken Ship | Water element + AoE |
| Orc Hero | 68 | Orc Dungeon | Earth Spike, summons Orc Warriors |
| Orc Lord | 78 | Orc Village | Massive AoE, summons |
| Moonlight Flower | 72 | Payon Cave | Teleport spam, high speed |
| Eddga | 75 | Payon Forest | Fire element, high ATK |
| Baphomet | 81 | Hidden Temple | Dark Strike, multi-target |
| Dark Lord | 80 | GH Church | Shadow element, Death skills |
| Stormy Knight | 85 | Clock Tower | Storm Gust, freeze AoE |

### 6.4 Mini-Boss Implementation

Mini-bosses spawn more frequently and have lesser rewards:

| Mini-Boss | Level | Zone | Drop Highlights |
|-----------|-------|------|----------------|
| Angeling | 45 | Fields | Angeling Card (all Holy) |
| Deviling | 50 | Fields | Deviling Card (-50% Neutral) |
| Mastering | 25 | Fields | +20% HP recovery card |
| Ghost Ring | 40 | Dungeons | Ghostring Card (Ghost armor) |
| Marine Sphere | 30 | Underwater | Self-destruct, high drops |

### 6.5 Drop Table System Enhancement

Current: chance-based loot rolling with pre-resolved itemIds. Enhancements needed:

| Feature | Description |
|---------|-------------|
| **Guaranteed drops** | Some items drop 100% of the time (Poring -> Jellopy) |
| **MVP drops** | Special drop list only for MVP killer |
| **Card drops** | 0.01% base rate, affected by LUK and drop rate modifiers |
| **Drop rate events** | Server-wide drop rate multiplier (2x, 3x events) |
| **Steal interaction** | Thief Steal skill accesses drop table with modified rates |
| **Item ID accuracy** | Verify all 846 resolved drops match iRO/kRO classic data |

### 6.6 Definition of Done -- Phase 4

- [ ] 100+ monster spawn points across 15+ zones
- [ ] Monster skill system functional with 10+ different skills
- [ ] 3+ MVP bosses spawning with unique mechanics
- [ ] 5+ mini-bosses spawning
- [ ] Drop tables verified against RO classic data
- [ ] MVP tombstone system working
- [ ] Spawn timer system for MVPs
- [ ] Tested: MVP killed by party (shared credit), drop distribution

---

## 7. Phase 5: Item and Equipment Deep Dive

**Goal**: Expand the item database to 1,000+ items, implement equipment refining, the card system, consumable effects, and weight enforcement.

**Estimated Tasks**: 50-60
**Dependencies**: Phase 1 (status effects for card effects), Phase 4 (drop sources)
**Duration Estimate**: 4-6 weeks

### 7.1 Item Database Expansion

Current: 148 items. Target: 1,000+.

| Category | Current | Target | Priority |
|----------|---------|--------|----------|
| Consumables | 28 | 60+ | HIGH |
| Etc/materials | 58 | 200+ | MEDIUM |
| Weapons | 20 | 150+ | HIGH |
| Armor (body) | 3+11 | 50+ | HIGH |
| Headgear (top/mid/low) | 5 | 100+ | MEDIUM |
| Shields | 1 | 20+ | HIGH |
| Footgear | 1 | 20+ | MEDIUM |
| Garments | 0 | 30+ | MEDIUM |
| Accessories | 1 | 50+ | MEDIUM |
| Cards | 23 | 200+ | MEDIUM |

**Key files to modify**: `database/init.sql`, new migration `add_expanded_items.sql`
**Script needed**: `scripts/generate_item_database.js` -- extract from rAthena item_db

### 7.2 Equipment Refining System

RO Classic refining mechanics:

| Feature | Description |
|---------|-------------|
| **NPC**: Hollgrehenn (weapon), unnamed (armor) | Refining NPCs in towns |
| **Materials**: Phracon (Lv1), Emveretarcon (Lv2), Oridecon (Lv3-4 weapon), Elunium (armor) | Required per refine attempt |
| **Zeny cost**: Varies by item level | 200-100,000 zeny per attempt |
| **Success rate**: Decreases per level | +1-4: safe. +5-7: 60-75%. +8-10: 20-40% |
| **Failure**: Item destroyed or downgraded | Depending on server config (official = destroyed) |
| **Bonus**: +ATK per refine (weapon), +DEF per refine (armor) | Formula varies by weapon level |
| **Visual**: +1-10 aura on character | Glow effect at high refine levels |

**Database changes**:
```sql
ALTER TABLE character_inventory ADD COLUMN refine_level INTEGER DEFAULT 0;
ALTER TABLE character_inventory ADD COLUMN card_slot_1 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN card_slot_2 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN card_slot_3 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN card_slot_4 INTEGER DEFAULT NULL;
ALTER TABLE items ADD COLUMN card_slots INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN weapon_level INTEGER DEFAULT 0;
```

**Socket.io events**:
| Event | Direction | Payload |
|-------|-----------|---------|
| `refine:attempt` | C->S | `{ inventoryId, materialId }` |
| `refine:result` | S->C | `{ success, inventoryId, newRefineLevel, itemDestroyed }` |

### 7.3 Card System

Cards are inserted into equipment slots to provide stat bonuses and special effects.

| Feature | Description |
|---------|-------------|
| **Card slots per item** | 0-4 slots depending on item |
| **Compound action** | Right-click card -> select target equipment with open slot |
| **Card removal** | Not possible without special NPC service |
| **Card effects** | Stat bonuses, race/size/element damage modifiers, status procs |
| **Stacking rules** | Same card in different slots stacks (some exceptions) |

**Implementation**:
1. Add `card_slots` to items table (0-4)
2. Add `card_slot_1` through `card_slot_4` to `character_inventory`
3. Create `server/src/ro_card_effects.js` -- mapping card_id to effect functions
4. On equip, aggregate card effects into player's effective stats
5. On auto-attack, check for proc effects (5% stun, drain HP, etc.)

**Card effect types**:
| Type | Example | Implementation |
|------|---------|----------------|
| Stat bonus | Poring Card: LUK +2, PD +1 | Add to effective stats on equip |
| Racial damage | +20% vs Brute | cardMods in damage formula (already supported) |
| Size damage | +15% vs Small | cardMods in damage formula (already supported) |
| Element damage | +5% vs Water | cardMods in damage formula (already supported) |
| Status proc | 5% stun on attack | Check on each auto-attack hit |
| Drain | 5% HP drain on attack | Calculate after damage, heal attacker |
| Skill grant | Creamy Card: Teleport Lv1 | Add skill to available skills |
| Auto-cast | Auto-cast Bolt Lv3 on attack | Trigger skill execution on hit |

### 7.4 Consumable Effect System Enhancement

Current: basic HP/SP restore. Need full consumable effects:

| Consumable Type | Effect | Example |
|----------------|--------|---------|
| HP potion | Instant HP restore | Red Potion: +50 HP |
| SP potion | Instant SP restore | Blue Potion: +60 SP |
| Status cure | Remove specific status | Green Herb: cure Poison |
| Buff food | Temporary stat boost | STR Food: +3 STR for 30 min |
| Scroll | Cast a spell | Wind Scroll: cast wind spell |
| Teleport | Fly Wing / Butterfly Wing | DONE |
| Speed potion | Increase ASPD | Awakening Potion: +15% ASPD |

### 7.5 Weight System Enforcement

| Weight Level | Threshold | Effect |
|-------------|-----------|--------|
| Normal | < 50% max weight | No penalty |
| Overweight 50% | >= 50% max weight | Cannot use HP/SP regen items |
| Overweight 90% | >= 90% max weight | Cannot move, cannot attack |

**Implementation**:
- Calculate total carried weight on inventory change
- Max weight = 2000 + STR * 30 (class modifiers apply)
- Send weight status in `inventory:data` payload
- Block item pickup if would exceed max weight
- Block movement if >= 90%

### 7.6 Equipment Visibility on Character

Equipment should visually change the character model:

| Slot | Visual Change | Priority |
|------|--------------|----------|
| Weapon | Weapon mesh in hand | HIGH |
| Headgear (top) | Head accessory mesh | MEDIUM |
| Headgear (mid) | Face accessory mesh | LOW |
| Headgear (low) | Lower face mesh | LOW |
| Armor | Body material/mesh swap | MEDIUM |
| Shield | Shield mesh on arm | MEDIUM |
| Garment | Cape/wing mesh | LOW |

This is primarily an art pipeline task (Phase 11) but the socket event infrastructure should be built here:
- `player:equipment_visual` event broadcasts equipped item IDs to other players
- Client maps item IDs to mesh/material assets

### 7.7 Definition of Done -- Phase 5

- [ ] 500+ items in database with accurate stats
- [ ] Refining system functional with success/failure
- [ ] Card compound system working (insert card into equipment)
- [ ] Card effects correctly modify stats and trigger procs
- [ ] Weight system enforced with 50%/90% thresholds
- [ ] All consumable types have effects
- [ ] Weapon mesh changes on equip (at least 5 weapon types visible)
- [ ] Tested: card + refined weapon damage calculation correct

---

## 8. Phase 6: Social Systems

**Goal**: Implement party, guild, chat, friend list, and social infrastructure that makes the game feel like an MMO.

**Estimated Tasks**: 60-70
**Dependencies**: Phase 1 (party EXP share requires combat), Phase 3 (zones for guild territory)
**Duration Estimate**: 4-6 weeks

### 8.1 Party System

| Feature | Priority |
|---------|----------|
| Party creation (leader invites) | HIGH |
| Party invite / accept / reject | HIGH |
| Party member list (HP/SP visible) | HIGH |
| Shared EXP (equal split or "each take" mode) | HIGH |
| Party leader transfer | MEDIUM |
| Party kick | MEDIUM |
| Party disbands when leader leaves | MEDIUM |
| Party chat channel | HIGH |
| Max 12 members per party | HIGH |
| Party member location on minimap | MEDIUM |
| Cross-zone party persistence | MEDIUM |

**Database**:
```sql
CREATE TABLE parties (
    party_id SERIAL PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    leader_character_id INTEGER REFERENCES characters(character_id),
    exp_share_mode VARCHAR(20) DEFAULT 'each_take',  -- 'each_take' or 'even_share'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE party_members (
    party_id INTEGER REFERENCES parties(party_id) ON DELETE CASCADE,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (party_id, character_id)
);
```

**Socket.io events**:
| Event | Direction | Payload |
|-------|-----------|---------|
| `party:create` | C->S | `{ name }` |
| `party:invite` | C->S | `{ targetCharacterId }` |
| `party:invite_received` | S->C | `{ partyId, partyName, inviterName }` |
| `party:accept` | C->S | `{ partyId }` |
| `party:reject` | C->S | `{ partyId }` |
| `party:leave` | C->S | `{}` |
| `party:kick` | C->S | `{ targetCharacterId }` |
| `party:data` | S->C | `{ partyId, name, leader, members: [{ id, name, hp, maxHp, sp, maxSp, zone }] }` |
| `party:member_update` | S->C | `{ characterId, hp, maxHp, sp, maxSp, zone, x, y }` |
| `party:chat` | C->S | `{ message }` |

**Client**: `PartySubsystem` (new UWorldSubsystem) + `SPartyWidget` (member list with HP bars)

### 8.2 Guild System

| Feature | Priority |
|---------|----------|
| Guild creation (requires Emperium item + 10,000 zeny) | HIGH |
| Guild invite / accept / reject | HIGH |
| Guild member list with rank display | HIGH |
| Guild ranks (1-20 with customizable names) | MEDIUM |
| Guild chat channel | HIGH |
| Guild emblem (upload image) | LOW |
| Guild skills (emergency call, guild buffs) | MEDIUM |
| Guild storage (shared bank) | MEDIUM |
| Guild notice/announcement board | LOW |
| Guild alliance / rivalry | LOW |
| Max 56 members per guild | HIGH |

**Database**:
```sql
CREATE TABLE guilds (
    guild_id SERIAL PRIMARY KEY,
    name VARCHAR(50) UNIQUE NOT NULL,
    master_character_id INTEGER REFERENCES characters(character_id),
    level INTEGER DEFAULT 1,
    exp INTEGER DEFAULT 0,
    max_members INTEGER DEFAULT 16,
    notice TEXT DEFAULT '',
    emblem BYTEA DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE guild_members (
    guild_id INTEGER REFERENCES guilds(guild_id) ON DELETE CASCADE,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    rank INTEGER DEFAULT 0,
    position_name VARCHAR(50) DEFAULT 'Member',
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (guild_id, character_id)
);

CREATE TABLE guild_storage (
    guild_id INTEGER REFERENCES guilds(guild_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    slot_index INTEGER DEFAULT -1,
    deposited_by INTEGER REFERENCES characters(character_id),
    deposited_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 8.3 Chat System Expansion

Current: Global chat only. Expand to:

| Channel | Scope | Format | Color |
|---------|-------|--------|-------|
| Global | All online | `[Global] Name: message` | White |
| Zone | Same zone | `Name: message` | Default |
| Party | Party members | `[Party] Name: message` | Green |
| Guild | Guild members | `[Guild] Name: message` | Yellow |
| Whisper | Direct player | `[From Name]: message` | Pink |
| Trade | All online | `[Trade] Name: message` | Orange |
| System | Server announcements | `[System] message` | Red |

### 8.4 Friend List

| Feature | Priority |
|---------|----------|
| Add friend (request/accept) | MEDIUM |
| Remove friend | MEDIUM |
| See online/offline status | MEDIUM |
| See current zone | LOW |
| Quick whisper from friend list | MEDIUM |

**Database**: `friend_list` table (character_id, friend_character_id, status)

### 8.5 Block/Ignore System

| Feature | Priority |
|---------|----------|
| Block player (no whispers, no trade requests) | MEDIUM |
| Unblock player | MEDIUM |
| Blocked players hidden from chat | MEDIUM |

### 8.6 Definition of Done -- Phase 6

- [ ] Party system fully functional (create, invite, join, leave, kick)
- [ ] Party EXP sharing works correctly
- [ ] Party HP/SP visible in party widget
- [ ] Guild system functional (create, invite, join, ranks)
- [ ] Guild chat working
- [ ] Guild storage functional
- [ ] All chat channels working (zone, party, guild, whisper, trade)
- [ ] Friend list with online status
- [ ] Block system prevents whispers
- [ ] Tested with 4+ players in a party

---

## 9. Phase 7: PvP and War of Emperium

**Goal**: Implement competitive PvP, guild-vs-guild combat, and the signature War of Emperium castle siege system.

**Estimated Tasks**: 40-50
**Dependencies**: Phase 6 (guild system), Phase 2 (all classes playable)
**Duration Estimate**: 4-6 weeks

### 9.1 PvP System

| Feature | Description |
|---------|-------------|
| **PvP Maps** | Specific zones flagged `pvp: true` in zone registry |
| **PvP Toggle** | Cannot attack other players in non-PvP zones |
| **PvP Damage Rules** | Reduced damage (60-70% of PvE damage) to balance TTK |
| **Death in PvP** | Respawn at save point, no EXP loss |
| **PvP Rankings** | Kill/death tracking per character |
| **PvP Arena** | Dedicated instance zone for structured PvP |

**RO-Specific PvP Modes**:
| Mode | Description |
|------|-------------|
| Free-for-all PvP | Open world PvP in marked zones |
| Yoyo Mode | Random teleport PvP arena |
| GvG (Guild vs Guild) | Guild-based team PvP |

### 9.2 War of Emperium

WoE is the signature RO end-game feature. Full implementation:

| Component | Description | Priority |
|-----------|-------------|----------|
| **Castle zones** | Special maps (5+ castles per realm) | HIGH |
| **Emperium** | Destructible crystal; destroy to capture castle | HIGH |
| **Schedule** | WoE occurs on set days/times (e.g., Sat/Sun 2pm-4pm) | HIGH |
| **Castle ownership** | Winning guild controls castle until next WoE | HIGH |
| **Castle economy** | Treasure boxes spawn for owning guild | MEDIUM |
| **Guardian NPCs** | Castle defenses (Knight, Soldier guardians) | MEDIUM |
| **Guild skills in WoE** | Emergency Call (teleport guild to leader), Strengthen | MEDIUM |
| **Barricades** | Destructible barriers inside castle | MEDIUM |
| **Traps** | Hunter traps in castle defense | LOW |

**Emperium mechanics**:
- HP = 3,245,880 (standard)
- No magic damage (physical only)
- Cannot be healed
- No miss (always hits)
- No critical hits
- Element: Holy 1

**WoE rules**:
- No Teleport/Fly Wing in castles
- Reduced item usage (no Yggdrasil Seed/Berry)
- Skills have reduced duration
- Guild leader can set guardian stone positions
- Treasure boxes appear at 00:00 daily for owning guild

### 9.3 Definition of Done -- Phase 7

- [ ] PvP zones functional with proper damage rules
- [ ] PvP kill tracking and ranking board
- [ ] WoE schedule system (configurable times)
- [ ] Emperium spawns in castle zones
- [ ] Emperium destructible by physical attacks only
- [ ] Castle ownership transfers on Emperium break
- [ ] Guardian NPCs defend castle
- [ ] Treasure box system for castle owners
- [ ] Emergency Call skill works in WoE
- [ ] Tested with 10+ players in WoE scenario

---

## 10. Phase 8: NPC and Quest System

**Goal**: Build a flexible NPC dialogue engine and quest tracking system supporting job change quests, access quests, and daily quests.

**Estimated Tasks**: 40-50
**Dependencies**: Phase 3 (zones for NPC placement), Phase 2 (job change NPCs)
**Duration Estimate**: 3-5 weeks

### 10.1 NPC Dialogue Engine

| Feature | Description |
|---------|-------------|
| **Dialogue tree** | JSON-defined conversation trees with branching choices |
| **NPC responses** | Text + portrait + optional sound |
| **Player choices** | 2-6 options per dialogue node |
| **Conditions** | Branch based on level, class, items, quest state |
| **Actions** | Give item, take item, give EXP, start quest, warp player |
| **Variables** | Per-player NPC state (talked before, quest progress) |

**Data format**:
```javascript
// server/src/npc_dialogues/kafra_prontera.js
{
    npcId: 'kafra_prontera_1',
    dialogue: {
        'start': {
            text: 'Welcome to the Kafra Corporation! How may I help you?',
            choices: [
                { text: 'Save', action: 'kafra:save' },
                { text: 'Teleport Service', next: 'teleport_menu' },
                { text: 'Storage', action: 'storage:open' },
                { text: 'Cancel', action: 'close' }
            ]
        },
        'teleport_menu': {
            text: 'Where would you like to go?',
            choices: [ /* ... */ ]
        }
    }
}
```

**Client**: `DialogueSubsystem` + `SDialogueWidget` -- shows NPC portrait, text, and choice buttons

### 10.2 Quest System

| Feature | Description |
|---------|-------------|
| **Quest database** | JSON definitions with objectives, rewards, prerequisites |
| **Quest states** | Not Started, In Progress, Complete, Failed |
| **Objective types** | Kill X monsters, Collect X items, Talk to NPC, Reach zone, Reach level |
| **Rewards** | EXP, zeny, items, skill points, unlocks |
| **Quest tracking** | Server-side progress per character |
| **Quest log UI** | `SQuestLogWidget` showing active/completed quests |
| **Quest markers** | Minimap indicators for quest NPCs (! for new, ? for turn-in) |

**Database**:
```sql
CREATE TABLE quest_definitions (
    quest_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    category VARCHAR(50) DEFAULT 'main', -- main, job_change, access, daily, side
    min_level INTEGER DEFAULT 1,
    required_class VARCHAR(20) DEFAULT NULL,
    prerequisite_quests INTEGER[] DEFAULT '{}',
    objectives JSONB NOT NULL,
    rewards JSONB NOT NULL
);

CREATE TABLE character_quests (
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    quest_id INTEGER REFERENCES quest_definitions(quest_id),
    state VARCHAR(20) DEFAULT 'in_progress', -- in_progress, complete, failed
    progress JSONB DEFAULT '{}',
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP DEFAULT NULL,
    PRIMARY KEY (character_id, quest_id)
);
```

### 10.3 Job Change Quests

Each class has a specific job change quest:

| Class | Quest Location | Requirements | Test |
|-------|---------------|-------------|------|
| Swordsman | Izlude Swordsman Guild | Novice JLv10, kill specific monsters | Reach NPC, answer questions, combat test |
| Mage | Geffen Magic Academy | Novice JLv10, collect gems | Quiz + element identification |
| Archer | Payon Archer Guild | Novice JLv10, hit target range | Shooting range mini-game |
| Acolyte | Prontera Church | Novice JLv10, collect specific items | Talk to multiple NPCs |
| Thief | Morroc Pyramid | Novice JLv10, reach mushroom NPC | Navigate dungeon, find NPC |
| Merchant | Alberta Merchant Guild | Novice JLv10, pay zeny | Quiz + item appraisal |

### 10.4 Access Quests

| Quest | Unlocks | Level |
|-------|---------|-------|
| Prontera Culvert | prt_dungeon_01-04 | 15+ |
| Clock Tower | Aldebaran Clock Tower | 40+ |
| Glast Heim | All Glast Heim maps | 60+ |
| Biolab | Lighthalzen Biolab | 80+ |
| Thor Volcano | Thor Dungeon | 75+ |

### 10.5 Definition of Done -- Phase 8

- [ ] NPC dialogue engine supports branching conversations
- [ ] Quest system tracks progress server-side
- [ ] Quest log UI shows active and completed quests
- [ ] All 6 first-class job change quests implemented
- [ ] At least 3 access quests implemented
- [ ] Quest NPCs have visual indicators (!, ?)
- [ ] Quests grant correct rewards on completion
- [ ] Tested: full job change quest flow for all 6 classes

---

## 11. Phase 9: Economy

**Goal**: Implement player-to-player trading, the vending system, and complete the economy loop.

**Estimated Tasks**: 30-40
**Dependencies**: Phase 5 (items), Phase 6 (social systems)
**Duration Estimate**: 3-4 weeks

### 11.1 Vending System

Merchants set up shops that other players can browse:

| Feature | Description |
|---------|-------------|
| **Open shop** | Merchant uses Vending skill, selects items + prices from inventory |
| **Vendor display** | Shop sign above character with shop name |
| **Browse** | Other players click vendor to see items |
| **Buy** | Select items, confirm purchase, zeny transfer |
| **Close shop** | Merchant manually closes or logs out |
| **Max items** | Vending level + 2 items per shop |

**Socket.io events**:
| Event | Direction | Payload |
|-------|-----------|---------|
| `vending:open` | C->S | `{ shopName, items: [{ inventoryId, price, quantity }] }` |
| `vending:browse` | C->S | `{ vendorCharacterId }` |
| `vending:data` | S->C | `{ vendorId, shopName, items: [{ itemName, price, quantity, stats }] }` |
| `vending:buy` | C->S | `{ vendorCharacterId, items: [{ inventoryId, quantity }] }` |
| `vending:sold` | S->C | `{ itemName, quantity, price, buyerName }` |
| `vending:close` | C->S | `{}` |

### 11.2 Trading System

Direct player-to-player trade:

| Feature | Description |
|---------|-------------|
| **Trade request** | Right-click player -> Trade |
| **Trade window** | Both players see two panels (my offer / their offer) |
| **Item placement** | Drag items from inventory to trade panel |
| **Zeny** | Enter zeny amount in trade |
| **Lock** | Both players must lock their offer |
| **Confirm** | Both players confirm after lock |
| **Cancel** | Either player can cancel at any time |
| **Anti-scam** | If either side changes after lock, both unlock |

### 11.3 Buying Store

Reverse vending -- player posts what they want to buy:

| Feature | Description |
|---------|-------------|
| **Open store** | Player sets up buying store with wanted items + max prices |
| **Seller interaction** | Other players can sell matching items directly |
| **Auto-purchase** | When seller confirms, items and zeny exchange automatically |

### 11.4 Storage System

| Feature | Description |
|---------|-------------|
| **Kafra storage** | 600 slot shared storage per account |
| **Guild storage** | 600 slot shared storage per guild |
| **Store/retrieve** | Drag items between inventory and storage |
| **Zeny deposit/withdraw** | Store zeny in storage |
| **Access** | Kafra NPCs in towns (100 zeny fee) |

**Database**:
```sql
CREATE TABLE kafra_storage (
    storage_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    refine_level INTEGER DEFAULT 0,
    card_slot_1 INTEGER DEFAULT NULL,
    card_slot_2 INTEGER DEFAULT NULL,
    card_slot_3 INTEGER DEFAULT NULL,
    card_slot_4 INTEGER DEFAULT NULL,
    slot_index INTEGER DEFAULT -1,
    stored_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 11.5 Economy Balancing

| Concern | Mitigation |
|---------|-----------|
| **Zeny inflation** | Monitor total zeny in circulation, adjust NPC prices |
| **Item duplication** | Server-authoritative inventory -- all transactions validated |
| **Price manipulation** | No auction house initially, free market vending |
| **Bot prevention** | CAPTCHA on trade, rate limits on vending |

### 11.6 Definition of Done -- Phase 9

- [ ] Vending system functional (open shop, browse, buy)
- [ ] Trading system with anti-scam protections
- [ ] Kafra storage working (store, retrieve, zeny)
- [ ] Guild storage accessible to guild members
- [ ] Economy transactions are atomic (no partial failures)
- [ ] Anti-dupe measures validated
- [ ] Tested with 3+ players trading simultaneously

---

## 12. Phase 10: Companion Systems

**Goal**: Implement pets, Homunculus, Falcon, Peco Peco mounts, and Merchant carts.

**Estimated Tasks**: 40-50
**Dependencies**: Phase 2 (class-specific systems), Phase 5 (taming items)
**Duration Estimate**: 4-6 weeks

### 12.1 Pet System

| Feature | Description |
|---------|-------------|
| **Taming** | Use taming item on specific monster species |
| **Egg/Incubator** | Pet stored as egg item, hatch with Incubator |
| **Hunger** | Pets get hungry over time, feed with specific food |
| **Intimacy** | 0-1000 scale based on feeding and time |
| **Loyalty bonus** | High intimacy grants stat bonuses |
| **Pet equipment** | Accessory slot for pets (one equippable item) |
| **Pet evolution** | Some pets evolve at max intimacy |
| **Following** | Pet follows player as NPC companion |
| **Auto-loot** | Some pets auto-loot items at high intimacy |

**Tameable pets** (initial set):
| Pet | Taming Item | Food | Loyal Bonus |
|-----|------------|------|-------------|
| Poring | Unripe Apple | Apple | LUK +2, Perfect Dodge +1 |
| Lunatic | Rainbow Carrot | Carrot | Critical Rate +3 |
| Drops | Orange Juice | Yellow Herb | HIT +3 |
| Poporing | Bitter Herb | Green Herb | LUK +2 |
| Chonchon | Rotten Fish | Pet Food | AGI +4 |
| Spore | Dew Laden Moss | Pet Food | HIT +5 |

### 12.2 Homunculus System (Alchemist)

| Feature | Description |
|---------|-------------|
| **Creation** | Alchemist skill: Call Homunculus (requires materials) |
| **Types** | 4 types: Lif (heal), Amistr (tank), Filir (DPS), Vanilmirth (magic) |
| **AI** | Autonomous combat companion with configurable behavior |
| **Leveling** | Homunculus gains EXP and levels independently |
| **Skills** | Each type has unique skills |
| **Loyalty** | Feed to maintain, loses intimacy if dies |
| **Evolution** | Evolve at level 99 with high intimacy |

### 12.3 Falcon System (Hunter)

| Feature | Description |
|---------|-------------|
| **Acquire** | Hunter guild NPC, 2,500 zeny |
| **Auto Blitz** | Chance to trigger Blitz Beat on auto-attack |
| **Blitz Beat** | Active skill: falcon attacks target |
| **Steel Crow** | Passive skill: increases falcon damage |
| **Visual** | Falcon perches on shoulder, flies during Blitz Beat |

### 12.4 Peco Peco Mount (Knight/Crusader)

| Feature | Description |
|---------|-------------|
| **Acquire** | Requires Riding skill + Peco Peco NPC |
| **Speed bonus** | +25% movement speed |
| **ASPD penalty** | -25% ASPD (reduced by Cavalry Mastery) |
| **Visual** | Character rides Peco Peco model |
| **Skill requirement** | Brandish Spear requires mount |
| **Dismount** | Toggle on/off |

### 12.5 Cart System (Merchant)

| Feature | Description |
|---------|-------------|
| **Acquire** | Requires Pushcart skill + Kafra cart rental |
| **Storage** | 100 extra inventory slots in cart |
| **Speed penalty** | -50% movement speed (reduced by Pushcart skill level) |
| **Visual** | Cart dragged behind character |
| **Vending requirement** | Vending requires cart |
| **Cart Revolution** | Blacksmith skill: AoE using cart weight |

### 12.6 Definition of Done -- Phase 10

- [ ] Pet system: taming, feeding, intimacy, stat bonuses
- [ ] At least 6 tameable pets
- [ ] Homunculus basic framework (1 type fully working)
- [ ] Falcon for Hunters (auto Blitz Beat + manual)
- [ ] Peco Peco mount for Knights (speed + ASPD mod)
- [ ] Cart for Merchants (extra storage + visual)
- [ ] All companions visible to other players

---

## 13. Phase 11: Art and Polish

**Goal**: Replace placeholder art with custom or stylized assets, create the RO Classic aesthetic in 3D.

**Estimated Tasks**: 100+ (ongoing)
**Dependencies**: All gameplay systems (art wraps around mechanics)
**Duration Estimate**: Ongoing throughout development, 8-12 weeks focused

### 13.1 Character Model Pipeline

| Task | Description | Priority |
|------|-------------|----------|
| **Base body mesh** | Male/female body with RO proportions (chibi or realistic) | HIGH |
| **Hair styles** | 19 hair styles per gender (matching RO selection) | HIGH |
| **Hair colors** | 9 color variations per style | HIGH |
| **Facial features** | Basic face customization | LOW |
| **Animation set** | Idle, walk, run, attack (per weapon type), sit, dead, cast, emote | HIGH |
| **Equipment meshes** | Modular equipment that attaches to body | MEDIUM |

### 13.2 Monster Models

| Tier | Monster Count | Model Quality | Priority |
|------|--------------|---------------|----------|
| Starter (Lv 1-15) | 15-20 | Full model + animations | HIGH |
| Mid-level (Lv 15-40) | 20-30 | Full model + animations | MEDIUM |
| High-level (Lv 40-70) | 20-30 | Full model + animations | MEDIUM |
| End-game (Lv 70-99) | 15-20 | Full model + animations | LOW |
| MVPs | 10-15 | High-quality model + unique animations | MEDIUM |

**Approach**: Start with stylized low-poly models (RO-inspired chibi style). Consider AI-assisted generation tools for base meshes, then hand-polish.

### 13.3 Environment Art

| Zone Type | Art Requirements | Priority |
|-----------|-----------------|----------|
| Towns | Buildings, plazas, fountains, NPC stands, signs | HIGH |
| Fields | Terrain, trees, rocks, grass, flowers, water features | HIGH |
| Dungeons | Cave walls, torches, cobwebs, bones, atmospheric lighting | MEDIUM |
| Interiors | NPC buildings (guild halls, churches, shops) | LOW |

### 13.4 Equipment Visual System

| Slot | Visual Implementation |
|------|----------------------|
| Weapon | Skeletal mesh socket attachment, swap on equip |
| Headgear | Static mesh socket on head bone |
| Shield | Static mesh socket on left arm |
| Armor | Material/mesh swap on body |
| Garment | Cape/wing attachment on back |

### 13.5 VFX Expansion

Current: VFX for ~10 skills. Target: VFX for all 86+ skills.

| VFX Category | Count | Approach |
|-------------|-------|----------|
| Bolt skills | 3 done | Template existing |
| AoE skills | 2 done | Create more Niagara systems |
| Buff effects | 3 done | Expand Cascade/Niagara library |
| Heal/Support | 1 done | Holy light effects |
| Physical attacks | 0 done | Slash/impact particles |
| Status effects | 2 done | Need 8 more |
| Environmental | 0 | Weather, ambient particles |

Reference: `docsNew/05_Development/VFX_Asset_Reference.md` catalogs 1,574 available VFX assets.

### 13.6 UI Art Polish

Target the RO Classic aesthetic:

| Element | Current | Target |
|---------|---------|--------|
| Window frames | Plain Slate boxes | RO-style brown wood frames |
| Buttons | Default UE5 | RO-style buttons with hover/press states |
| Icons | Placeholder text | Hand-drawn 24x24 icons per item/skill |
| Fonts | Default | RO-style pixel font (or clean sans-serif) |
| HP/SP bars | Colored rectangles | RO-style bars with gradients |
| Minimap | None | Circular minimap with icons |
| Cursor | Default | RO-style custom cursor |

### 13.7 Definition of Done -- Phase 11

- [ ] Player character model with 19 hair styles and 9 colors
- [ ] Male and female body variants
- [ ] 20+ monster models with idle/attack/death animations
- [ ] 5+ weapon meshes visible on character
- [ ] 10+ headgear meshes visible on character
- [ ] RO-style UI theme applied to all windows
- [ ] 100+ item/skill icons created
- [ ] VFX for 50+ skills
- [ ] All status effects have visual indicators

---

## 14. Phase 12: Audio

**Goal**: Implement BGM, combat SFX, skill SFX, UI sounds, and ambient audio to create the RO atmosphere.

**Estimated Tasks**: 30-40
**Dependencies**: Phase 3 (zones for BGM assignment), Phase 1 (skills for SFX)
**Duration Estimate**: 3-4 weeks

### 14.1 Background Music

| Zone Type | BGM Style | Example |
|-----------|-----------|---------|
| Prontera | Bright medieval orchestra | Theme_of_Prontera.ogg |
| Geffen | Mystical/magical | Ancient_Groover.ogg |
| Payon | Asian/Japanese | Payon_Theme.ogg |
| Morroc | Desert/Arabian | Desert_Theme.ogg |
| Dungeons | Dark/tense ambient | Dungeon_Theme.ogg |
| Boss fights | Epic orchestral | Boss_Battle.ogg |
| Login screen | Nostalgic/calm | Login_Theme.ogg |
| Character select | Gentle music box | Select_Theme.ogg |

**Implementation**: `AudioSubsystem` (UWorldSubsystem) that plays BGM based on current zone, crossfades on zone change.

### 14.2 Combat Sound Effects

| SFX Category | Count Needed | Examples |
|-------------|-------------|---------|
| Weapon attacks | 10 (per weapon type) | Slash, stab, blunt, arrow, spell cast |
| Skill sounds | 30+ (per skill) | Fire bolt whoosh, heal chime, lightning crack |
| Hit impacts | 5 | Flesh hit, armor hit, critical hit, miss swoosh |
| Death sounds | 3 | Player death, monster death, boss death |
| Status effects | 10 | Poison drip, stun stars, freeze crack |

### 14.3 UI Sound Effects

| SFX | Trigger |
|-----|---------|
| Button click | Any UI button press |
| Window open/close | Inventory, equipment, skill tree toggle |
| Item equip | Equipping any gear |
| Item use | Using consumable |
| Level up | Base or job level up |
| Quest complete | Quest completion |
| Error | Invalid action |
| Chat message | Incoming whisper |

### 14.4 Ambient Audio

| Zone Type | Ambient | Examples |
|-----------|---------|---------|
| Town | Crowd chatter, birds, fountain | Prontera market ambience |
| Field | Wind, birds, insects | Open grassland ambience |
| Forest | Dense birds, rustling leaves | Payon forest ambience |
| Dungeon | Dripping water, echoes, distant moans | Culvert ambience |
| Desert | Wind, sand, heat shimmer | Morroc ambience |
| Indoor | Muffled outside, fireplace | Guild hall ambience |

### 14.5 3D Spatialization

| Feature | Description |
|---------|-------------|
| **Skill sounds** | Positioned at cast/impact location |
| **Monster sounds** | Idle/attack sounds from monster position |
| **Footsteps** | Per-surface type (grass, stone, wood, water) |
| **Attenuation** | Sound volume decreases with distance |
| **Occlusion** | Walls muffle sounds (UE5 built-in) |

### 14.6 Definition of Done -- Phase 12

- [ ] BGM plays per zone, crossfades on transition
- [ ] 10+ combat SFX (per weapon type)
- [ ] 20+ skill SFX
- [ ] All UI interactions have sound feedback
- [ ] Ambient audio per zone type
- [ ] 3D spatialized combat sounds
- [ ] Volume controls in settings (Master, BGM, SFX)

---

## 15. Phase 13: Optimization and Launch Prep

**Goal**: Optimize client and server performance for 100+ concurrent players, harden security, and prepare for public launch.

**Estimated Tasks**: 40-50
**Dependencies**: All other phases substantially complete
**Duration Estimate**: 4-6 weeks

### 15.1 Server Performance Optimization

| Area | Target | Technique |
|------|--------|-----------|
| **Combat tick** | < 1ms per tick with 200 players | Spatial partitioning, skip idle enemies |
| **Enemy AI** | < 5ms per tick with 500 enemies | Zone-based activation, sleep inactive zones |
| **Position sync** | < 2ms per broadcast | Interest management (only send nearby players) |
| **Database queries** | < 10ms per query | Connection pooling, prepared statements, caching |
| **Memory usage** | < 1GB with 500 players | Object pooling server-side, trim unused data |
| **Socket.io** | Handle 500 connections | Multiple workers with Redis adapter |

**Interest Management** (critical for scale):
```
Current: broadcastToZone() sends to ALL players in zone
Target: broadcastToNearby() sends only to players within ~2000 UE units
Benefit: O(nearby) instead of O(zone_population) per event
```

### 15.2 Client Performance Optimization

| Area | Target | Technique |
|------|--------|-----------|
| **FPS** | > 60 FPS with 50 players on screen | LOD, culling, instancing |
| **Draw calls** | < 500 per frame | Mesh instancing for similar enemies |
| **Character rendering** | < 50 characters visible | Interest management culling |
| **Particle systems** | Budget per frame | Particle LOD, disable off-screen |
| **Network bandwidth** | < 50 KB/s per client | Delta compression, reduce update frequency |
| **Memory** | < 4 GB client RAM | Streaming levels, texture streaming |
| **Load times** | < 10s zone transition | Async loading, preload adjacent zones |

### 15.3 Load Testing

| Test | Target | Tool |
|------|--------|------|
| 100 concurrent logins | No crashes, < 5s auth time | Artillery + custom bots |
| 100 concurrent players in one zone | Server tick < 50ms | Custom bot scripts |
| 500 total concurrent (spread across zones) | No OOM, < 100ms latency | Distributed bot farm |
| MVP boss fight with 20 attackers | Correct damage, drops, no desync | Manual + automated |
| 50 concurrent trades | No duplication, atomic transactions | Custom trade bots |

### 15.4 Security Audit

| Vector | Mitigation | Status |
|--------|-----------|--------|
| **Speed hacking** | Server-side position validation | TODO (Phase 0) |
| **Damage hacking** | All damage calculated server-side | DONE |
| **Item duplication** | Atomic DB transactions, server-authoritative inventory | DONE |
| **Packet replay** | JWT expiry, nonce on critical events | PARTIAL |
| **SQL injection** | Parameterized queries throughout | DONE |
| **DoS** | Rate limiting on REST + Socket.io | PARTIAL |
| **Authentication bypass** | JWT validation on all socket events | DONE |
| **Memory editing** | Client is presentation-only | DONE (by design) |

### 15.5 Anti-Cheat Hardening

| Measure | Description |
|---------|-------------|
| **Speed validation** | Max distance between position updates based on ASPD/movement speed |
| **Cooldown enforcement** | Server tracks all skill cooldowns, rejects premature casts |
| **Damage validation** | Compare client-claimed damage vs server calculation (already server-auth) |
| **AFK detection** | Auto-disconnect after 30 min inactivity |
| **Multi-boxing detection** | Same IP limits (configurable) |
| **Bot detection** | Captcha on suspicious behavior patterns |

### 15.6 Database Optimization

| Task | Impact |
|------|--------|
| **Vacuum/analyze** schedule | Keep query planner stats fresh |
| **Partition large tables** | `character_inventory` by character_id range |
| **Archive old data** | Move deleted characters to archive table |
| **Read replicas** | PostgreSQL streaming replication for read queries |
| **Redis optimization** | Eviction policy, memory limits, persistence config |

### 15.7 Network Bandwidth Optimization

| Technique | Description |
|-----------|-------------|
| **Delta compression** | Only send changed fields in position updates |
| **Update throttling** | 10 Hz for distant players, 30 Hz for nearby |
| **Binary protocol** | Consider MessagePack instead of JSON for hot paths |
| **Event batching** | Combine multiple small events into one packet |
| **Zone interest** | Don't send events from adjacent zones |

### 15.8 Definition of Done -- Phase 13

- [ ] Server handles 100 concurrent players without degradation
- [ ] Client maintains 60 FPS with 50 on-screen characters
- [ ] Load test passes all targets
- [ ] Security audit complete with no critical vulnerabilities
- [ ] Anti-cheat detects and blocks speed/position hacking
- [ ] Database optimized with proper indexes and monitoring
- [ ] Network bandwidth under 50 KB/s per client at full load
- [ ] Monitoring dashboard for server health (CPU, memory, connections, tick time)

---

## 16. Cross-Cutting Concerns

These concerns apply to EVERY phase and must be addressed continuously.

### 16.1 Multiplayer Testing Protocol

**Every feature MUST be tested with 2+ PIE instances before merge.**

| Test | Frequency | Method |
|------|-----------|--------|
| 2-player basic functionality | Every feature | UE5 PIE with 2 instances |
| 5-player stress test | Every phase completion | 5 PIE instances or network clients |
| Cross-zone interaction | Every zone-related change | Test zone transitions with multiple players |
| Concurrent inventory operations | Every inventory change | 2 players trading/dropping simultaneously |
| Combat synchronization | Every combat change | 2 players attacking same target |

### 16.2 Server-Authoritative Validation

**NEVER trust client-sent values for authoritative data.**

| Data | Authority | Validation |
|------|-----------|-----------|
| Position | Server validates speed | Reject teleporting/speed hacking |
| Damage | Server calculates | Client only displays results |
| Inventory | Server tracks | Client sends requests, server confirms |
| Stats | Server calculates | Client displays server-provided values |
| Skill usage | Server validates | Check cooldowns, SP, range, status server-side |
| Trading | Server mediates | Atomic transactions, validate item ownership |
| EXP/Level | Server grants | Client cannot modify level data |

### 16.3 Error Handling Standards

**Server-side**:
```javascript
// Every socket handler follows this pattern:
socket.on('event:name', async (data) => {
    try {
        // Validate input
        if (!data || !data.requiredField) {
            socket.emit('event:error', { message: 'Invalid input' });
            return;
        }
        // Process
        // ...
        // Respond
        socket.emit('event:result', { success: true, ... });
    } catch (err) {
        logger.error(`[event:name] Error: ${err.message}`, { stack: err.stack });
        socket.emit('event:error', { message: 'Server error' });
    }
});
```

**Client-side (C++)**:
```cpp
// Every subsystem that binds socket events:
void UMySubsystem::OnEventReceived(const FString& EventName, const TSharedPtr<FJsonObject>& Data)
{
    if (!Data.IsValid()) { UE_LOG(LogTemp, Warning, TEXT("Null data in %s"), *EventName); return; }
    // Process...
}
```

### 16.4 Database Migration Strategy

| Rule | Description |
|------|-------------|
| **Forward-only** | Never modify existing migration files |
| **Naming** | `add_<feature>.sql` or `alter_<table>_<change>.sql` |
| **Idempotent** | Use `IF NOT EXISTS`, `ON CONFLICT DO NOTHING` |
| **Rollback plan** | Comment block at top with rollback SQL |
| **Documentation** | Update `docsNew/01_Architecture/Database_Architecture.md` |

### 16.5 Git Workflow

| Practice | Description |
|----------|-------------|
| **Branch per feature** | `feature/<phase>-<name>` (e.g., `feature/p1-status-effects`) |
| **Commit messages** | `feat:`, `fix:`, `refactor:`, `docs:`, `test:` prefixes |
| **No binary commits** | `.uasset` files not tracked (in `.gitignore`) |
| **PR review** | Self-review checklist before merge |
| **Version tagging** | Tag after each phase completion (e.g., `v0.1-phase0`) |

### 16.6 Documentation Protocol

After EVERY code change:
1. Update relevant `docsNew/` file
2. Update `MEMORY.md` if new architectural knowledge
3. Update socket event reference if new events added
4. Update database architecture doc if schema changed

---

## 17. Companion Document Index

The following companion documents provide deep-dive specifications for each major system. They should be created alongside or ahead of their corresponding phases.

| Document | Phase | Covers |
|----------|-------|--------|
| `01_Stats_Leveling_JobSystem.md` | 1, 2 | Base/job stats, EXP tables, level-up formulas, stat point allocation, class progression chains, ASPD per-class tables, HP/SP per-class formulas |
| `02_Combat_System.md` | 1 | Physical/magical damage formulas, HIT/FLEE, critical system, element table, size penalties, DEF/MDEF calculation, auto-attack loop, skill damage pipeline |
| `03_Skills_Complete.md` | 1, 2 | All 86+ skill definitions, cast times, cooldowns, SP costs, damage multipliers, status effect application, skill handler patterns, VFX mapping |
| `04_Monsters_EnemyAI.md` | 4 | All 509 monster templates, AI state machine, aggro system, assist behavior, mode flags, monster skills, MVP mechanics, spawn management |
| `05_Items_Equipment_Cards.md` | 5 | Complete item database schema, equipment slots, refining system, card compound system, card effect registry, weight system, consumable effects |
| `06_World_Maps_Zones.md` | 3 | All zone definitions, warp network graph, zone flags, spawn point configurations, Kafra NPC locations, dungeon floor connections |
| `07_NPCs_Quests_Shops.md` | 8, 9 | NPC dialogue engine spec, quest system schema, job change quest scripts, access quest requirements, shop NPC inventories |
| `08_PvP_Guild_WoE.md` | 7 | PvP damage rules, arena mechanics, guild system schema, WoE schedule, castle maps, Emperium mechanics, guardian NPCs |
| `09_UI_UX_System.md` | All | All UI widgets (Slate + UMG), subsystem inventory, Z-order map, hotkey assignments, drag-drop system, tooltip system |
| `10_Art_Animation_VFX_Pipeline.md` | 11 | Character model spec, monster model pipeline, environment art style guide, equipment mesh system, VFX asset catalog, animation requirements |
| `11_Multiplayer_Networking.md` | 0, 13 | Socket.io event catalog, position sync protocol, interest management, bandwidth optimization, server tick architecture |
| `12_Pets_Homunculus_Companions.md` | 10 | Pet taming/feeding/intimacy system, Homunculus AI/leveling, Falcon mechanics, Peco mount system, Cart system |
| `13_Economy_Trading_Vending.md` | 9 | Vending system spec, trade protocol, buying store spec, storage system, zeny flow analysis, anti-dupe measures |
| `14_Audio_Music_SFX.md` | 12 | BGM per zone map, SFX catalog, 3D spatialization config, volume control system, audio asset requirements |

---

## Phase Dependency Graph

```
Phase 0 (Foundation)
    |
    +--- Phase 1 (Combat) -----+--- Phase 2 (Classes)
    |       |                   |       |
    |       |                   |       +--- Phase 7 (PvP/WoE) [needs guilds from P6]
    |       |                   |
    |       +--- Phase 4 (Monsters) [needs zones from P3]
    |       |
    +--- Phase 3 (World) ------+--- Phase 8 (NPCs/Quests)
    |                           |
    +--- Phase 5 (Items) ------+--- Phase 9 (Economy)
    |                           |
    +--- Phase 6 (Social) -----+--- Phase 7 (PvP/WoE)
    |                           |
    +--- Phase 10 (Companions) [needs classes from P2, items from P5]
    |
    Phase 11 (Art) ---- runs alongside all phases
    Phase 12 (Audio) -- runs alongside all phases
    Phase 13 (Optimization) -- runs at end, after all gameplay systems
```

## Recommended Execution Order

Given that this is a solo developer project, the recommended approach is to work in focused sprints, completing one phase at a time with art and audio work interspersed:

1. **Phase 0** -- Foundation Hardening (2-3 weeks)
2. **Phase 1** -- Core Combat Completion (4-6 weeks)
3. **Phase 5** -- Item & Equipment Deep Dive (4 weeks, overlaps naturally with combat)
4. **Phase 2** -- Class System (6-8 weeks)
5. **Phase 3** -- World Expansion (8+ weeks, ongoing)
6. **Phase 4** -- Monster System Expansion (4 weeks, concurrent with Phase 3)
7. **Phase 6** -- Social Systems (4-6 weeks)
8. **Phase 8** -- NPC & Quest System (3-5 weeks)
9. **Phase 9** -- Economy (3-4 weeks)
10. **Phase 10** -- Companion Systems (4-6 weeks)
11. **Phase 7** -- PvP & War of Emperium (4-6 weeks)
12. **Phase 11** -- Art & Polish (ongoing, 8-12 weeks focused)
13. **Phase 12** -- Audio (3-4 weeks)
14. **Phase 13** -- Optimization & Launch Prep (4-6 weeks)

**Total estimated development time**: 60-90 weeks (15-22 months) for a solo developer working full-time.

---

## Milestone Definitions

| Milestone | Phases Complete | Playable State |
|-----------|----------------|---------------|
| **Alpha 0.1** | 0, 1 | Combat works with all first-class skills, 4 zones |
| **Alpha 0.2** | 0, 1, 2, 5 | All classes playable, items/equipment complete |
| **Alpha 0.3** | 0-6 | Social systems, 15+ zones, monster variety |
| **Beta 0.5** | 0-9 | Full gameplay loop: level, quest, trade, PvP |
| **Beta 0.8** | 0-12 | Art polished, audio complete, companions |
| **Release 1.0** | 0-13 | Optimized, secure, launch-ready |

---

## Summary of Key Metrics

| Metric | Current | Alpha Target | Release Target |
|--------|---------|-------------|----------------|
| Server lines of code | 8,421 (monolith) | 8,500+ (modular) | 15,000+ |
| C++ source files | 23 core + 30 UI | 40 core + 50 UI | 60+ core + 80 UI |
| Socket.io events | 33 | 60+ | 120+ |
| Skill definitions | 86 | 86 (all defined) | 86 + transcendent |
| Skill handlers (server) | 15 | 50+ | 86+ |
| Zones | 4 | 15+ | 30+ |
| Monster spawns | 46 | 200+ | 500+ |
| Items in database | 148 | 500+ | 1,000+ |
| DB tables | 4 core + hotbar | 10+ | 15+ |
| DB migrations | 10 | 15+ | 25+ |
| UE5 Levels | 4 | 15+ | 30+ |
| Slate subsystems | 14 | 20+ | 25+ |
| Concurrent player support | 5 tested | 20 tested | 100+ tested |

---

**This document is the definitive roadmap for the Sabri_MMO project. It should be updated as phases are completed, priorities shift, and new requirements emerge. Every implementation decision should reference this plan to ensure alignment with the overall architecture and phasing strategy.**

**Last Updated**: 2026-03-08
**Status**: Active
**Next Phase**: Phase 0 (Foundation Hardening)
