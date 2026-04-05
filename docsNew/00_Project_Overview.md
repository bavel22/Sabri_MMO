# Sabri_MMO — Project Overview

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [README / Quick Start](high-level%20docs/README.md) | [Global Rules](00_Global_Rules/Global_Rules.md)

## Executive Summary

**Sabri_MMO** is a class-based action MMORPG built as a solo developer project. It combines an Unreal Engine 5.7 client (C++ + Blueprints) with a Node.js backend (Express + Socket.io), PostgreSQL for persistence, and Redis for real-time caching. The architecture is **server-authoritative** — all combat, stats, inventory, and position logic is validated server-side.

## Technology Stack

| Layer | Technology | Version |
|-------|-----------|---------|
| **Game Client** | Unreal Engine 5.7 | 5.7 (C++ 17/20 + Blueprint) |
| **Web Framework** | Express.js | 4.18.2 |
| **Real-Time** | Socket.io | 4.8.3 |
| **Database** | PostgreSQL | 15.4 |
| **Cache** | Redis | 7.2+ (client v5.10.0) |
| **Auth** | JSON Web Tokens | jsonwebtoken 9.0.3 |
| **Password Hash** | bcrypt | 5.1.1 |
| **Rate Limiting** | express-rate-limit | 8.2.1 |

## Architecture Diagram

```
┌──────────────────────────┐         ┌──────────────────────────┐         ┌─────────────────┐
│     UE5.7 Client         │  HTTP   │     Node.js Server       │  SQL    │   PostgreSQL    │
│                          │◄───────►│                          │◄───────►│                 │
│ • C++ Core Classes       │         │ • Express REST API       │         │ • users         │
│ • Blueprint Game Logic   │ Socket  │ • Socket.io Events       │         │ • characters    │
│ • UMG Widget UI          │◄═══════►│ • Combat Tick Loop       │  Redis  │ • items         │
│ • SocketIOClient Plugin  │  .io    │ • Enemy AI Loop          │◄───────►│ • char_inventory│
│ • HttpModule (REST)      │         │ • JWT Auth Middleware     │  Cache  │                 │
└──────────────────────────┘         └──────────────────────────┘         └─────────────────┘
```

## Implemented Systems

### Authentication & Characters
- JWT-based login/register via REST API
- Character CRUD (create, list, select, soft-delete with password confirmation)
- Server list endpoint (`GET /api/servers`) with population tracking
- Character customization: hair style (1-19), hair color (0-8), gender
- 9 character slots per account, globally unique names (case-insensitive)
- JWT validation on `player:join` socket event (character ownership check)
- `FCharacterData` C++ struct (30+ fields) + `FServerInfo` struct
- `UMMOGameInstance` persists auth state, server selection, remembered username across levels
- Configurable server URL (no hardcoded localhost)

### Real-Time Multiplayer
- Socket.io bidirectional communication
- `player:join` / `player:position` / `player:moved` / `player:left` events
- Redis position caching with 5-minute TTL
- Remote player spawning, interpolation, and name tags
- Tested with 5+ concurrent players

### Combat System (Ragnarok Online-Style)
- Server-authoritative auto-attack loop (50ms tick)
- ASPD-based attack intervals (0–190 scale)
- Player-vs-Player and Player-vs-Enemy targeting
- Range checking with tolerance padding
- Death/respawn cycle with HP restore and teleport
- Kill messages broadcast in COMBAT chat channel
- **Element table**: 10 elements × 10 elements × 4 defense levels (rAthena pre-renewal canonical, 537 tests)
- **Size penalties**: 18 weapon types × 3 sizes (physical attacks only, not magic)
- **Card modifiers**: Per-category multiplicative stacking (race × element × size)
- `ro_damage_formulas.js`: Physical/magical damage, HIT/FLEE, Critical, Perfect Dodge, element/size lookups

### Stat System (RO-Style)
- 6 base stats: STR, AGI, VIT, INT, DEX, LUK
- Derived stats: statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, ASPD, maxHP, maxSP
- Stat point allocation with server validation
- Stats loaded from DB on join, saved on disconnect

### Enemy/NPC System
- **509 Ragnarok Online monsters** from rAthena pre-renewal database (auto-generated)
- **46 spawn points** active (zones 1-3 only) — zones 4-9 disabled pending higher-level content
- **Enemy sprite system**: Monsters can use the same `SpriteCharacterActor` as players — server sends `spriteClass`/`weaponMode` in `enemy:spawn`, client spawns animated sprite actors with walk/attack/hit/death animations
- RO-style wandering AI (random movement within spawn radius)
- **NavMesh pathfinding**: Server-side pathfinding using `recast-navigation` v0.42.1 (Recast/Detour). UE5 NavMesh exported as OBJ per zone, loaded at server startup. Enemies navigate around walls and obstacles instead of straight-line movement. Binary navmesh cache for fast restarts. Graceful fallback to straight-line when no navmesh available. De-aggro at 1200 units (MVPs exempt).
- Server-side health tracking, death, and timed respawn
- **126 RO drop items** integrated with inventory system (consumables, weapons, armor, cards, etc.)
- **15 existing game items** added as extra drops on appropriate monsters
- Drop tables with chance-based loot rolling using pre-resolved itemIds

### Items & Inventory
- **6,169 rAthena canonical items** (migrated from original 148 to full pre-renewal database)
  - Weapons, armor, headgear, footgear, garments, shields, accessories, consumables, etc, ammo, cards
  - **538 monster cards** (IDs 4001-4499): rAthena canonical data, flat stat bonuses, offensive combat mods (race/ele/size %), defensive mods, armor element changes
- PostgreSQL `items` (definitions) + `character_inventory` (per-character)
- Socket.io events: load, use consumable, equip/unequip, drop/discard
- Equipped weapon modifies ATK, range, and ASPD
- Stackable items with max stack limits
- **RO item name → ID mapping** for efficient drop processing
- **Card compound system** (`card:compound` event, server-validated): permanent card insertion into slotted equipment
- **Card compound UI** (`SCardCompoundPopup`, Z=23): double-click card in Etc tab opens centered popup with eligible unequipped equipment, slot diamonds, click-to-compound
- **Card bonus aggregation**: `rebuildCardBonuses()` recalculates on equip/join/compound — flat stats, offensive mods, defensive mods, armor element
- **Card combat modifiers**: offensive card bonuses applied at damage pipeline Step 6, defensive card bonuses at Step 8c
- **Armor element cards**: Ghostring, Pasana, Swordfish, Sandman, etc. change equipped armor's element for defense calculations
- **Weight threshold system** (RO Classic): <50% normal, 50-89% regen stops, >=90% attack/skills blocked, >100% no loot pickup. Cached `player.currentWeight`, `weight:status` event

### Chat System
- Global chat channel via Socket.io
- `chat:message` (client→server) / `chat:receive` (server→all)
- COMBAT channel for kill messages
- Expandable architecture (ZONE, PARTY, GUILD, TELL planned)

### Movement & Camera
- Top-down click-to-move with NavMesh pathfinding
- WASD keyboard controls (cancels click-to-move)
- Spring Arm camera with right-click rotation
- Mouse scroll zoom (200–1500 units)
- Character faces movement direction

### UI System — Pure C++ Slate (34 UWorldSubsystems)

> **Note:** All 23 UMG Blueprint widgets (WBP_*) and all Blueprint actor components (AC_HUDManager, AC_TargetingSystem, AC_CameraController) have been replaced by C++ Slate widgets + UWorldSubsystems. BP_GameFlow was replaced by LoginFlowSubsystem. The Blueprint assets may still exist in the Content Browser but are fully dead code.

#### Login Flow (Pure C++ Slate)
- `ULoginFlowSubsystem` — UWorldSubsystem state machine: Login → ServerSelect → CharacterSelect → CharacterCreate → EnteringWorld. Supports return-from-game path (ESC menu → CharacterSelect, skips login/server). Fullscreen background widget at Z=200 covers game UI.
- `SLoginWidget` — Username/password, remember username, error display, Enter/Tab keyboard shortcuts
- `SServerSelectWidget` — Scrollable server list with population/status, selection highlighting
- `SCharacterSelectWidget` — 3x3 card grid + detail panel (HP/SP bars, 6 stats, EXP), delete confirmation with password
- `SCharacterCreateWidget` — Name field, gender toggle, hair style/color pickers (arrow buttons), locked Novice class
- `SLoadingOverlayWidget` — Dimmed fullscreen overlay with "Please Wait" dialog and progress bar

#### Game HUD (Pure C++ Slate — 33 UWorldSubsystems)
- `SWorldHealthBarOverlay` — Floating HP/SP bars above characters (Z=8)
- `STargetFrameWidget` — Auto-attack target name + HP bar, RO brown/gold theme (Z=9, via CombatActionSubsystem)
- `SBasicInfoWidget` — Draggable HUD panel: player name, job, HP/SP bars, base/job EXP bars, weight, zuzucoin (Z=10)
- `SBuffBarWidget` — Buff/status icons with 3-letter abbreviations and countdown timers (Z=11)
- `SCombatStatsWidget` — ATK/DEF/HIT/FLEE/ASPD stat panel, F8 toggle (Z=12)
- `SInventoryWidget` — Item grid with drag-drop, equip/use/drop (Z=14)
- `SEquipmentWidget` — Equipment slots display (Z=15)
- `SHotbarRowWidget` — 4-row skill/item hotbar with keybinds (Z=16)
- `SShopWidget` — NPC shop buy/sell interface (Z=18)
- `SKafraWidget` — Kafra service dialog (Z=19)
- `SSkillTreeWidget` — Skill tree with point allocation (Z=20)
- `SDamageNumberOverlay` — Floating damage/heal numbers (Z=20)
- `SCardCompoundPopup` — Double-click card compound: equipment list with slot diamonds (Z=23)
- `SIdentifyPopup` — Item Appraisal: unidentified item list with generic names, one-per-cast (Z=24)
- `SCartWidget` — Cart inventory: 10-column grid, weight bar, drag-drop to/from inventory, F10 toggle (Z=14, via CartSubsystem)
- `SVendingSetupPopup` — Vending shop setup: cart item list, price entry, shop title (Z=24, via VendingSubsystem)
- `SVendingBrowsePopup` — Vending shop browse: dual-mode (buyer view with quantity input + vendor self-view with sale log and Close Shop), live stock updates, movement lock while vending (Z=24, via VendingSubsystem)
- `SCastBarOverlay` — World-projected cast time bars (Z=25)
- `SDeathOverlayWidget` — "You have been defeated" + Respawn button (Z=40, via CombatActionSubsystem)
- `SEscapeMenuWidget` — RO Classic "Select Option" popup: Character Select, Hotkey, Exit, Cancel. Shows Save Point when dead. ESC key toggle (Z=40, via EscapeMenuSubsystem)

#### Game Subsystems (Non-Widget, Pure C++)
- `CombatActionSubsystem` — 10 combat event handlers, bOrientRotationToMovement toggling, PlayAttackAnimation via reflection, target frame + death overlay
- `TargetingSubsystem` — 30Hz hover trace, cursor switching (Enemy=Crosshairs, NPC=TextEditBeam), hover indicators, pauses during skill targeting
- `PlayerInputSubsystem` — Click-to-move, click-to-attack (emit only), click-to-interact, walk-to-NPC/enemy
- `CameraSubsystem` — Right-click yaw rotation, scroll zoom (200-1500 units), fixed -55 degree pitch
- `MultiplayerEventSubsystem` — Outbound emit helpers only (0 bridges; all inbound events handled by dedicated subsystems)
- `PositionBroadcastSubsystem` — 30Hz position broadcasting via persistent socket
- `SkillVFXSubsystem` — Skill visual effects via Niagara
- `EnemySubsystem` — Enemy entity registry (TMap<int32, FEnemyEntry>), 5 event handlers, spawns BP_EnemyCharacter
- `OtherPlayerSubsystem` — Other player entity registry (TMap<int32, FPlayerEntry>), 2 event handlers
- `NameTagSubsystem` — Single OnPaint overlay renders ALL entity name tags (Z=7)
- `ChatSubsystem` — Chat window: chat:receive + 8 combat log events, 3 tabs (Z=13)
- `CartSubsystem` — Cart inventory management: cart:data/error/equipped handlers, cart:load re-request, drag-drop integration (Z=14)
- `VendingSubsystem` — Vending shop system: 7 socket handlers (setup/browse/buy/sell/close), shop sign via NameTagSubsystem, vendor self-view with live sale updates, movement lock, buyer quantity input, click-to-browse via PlayerInputSubsystem (Z=24)
- `ItemInspectSubsystem` — Item inspection detail panel: `SItemInspectWidget` with full item stats, card slots, refine info
- `PartySubsystem` — Party management: `SPartyWidget` with HP bars, context menu, invite popup (Z=12)
- `CraftingSubsystem` — Pharmacy/crafting UI: `SCraftingPopup` for Alchemist crafting
- `SummonSubsystem` — Summon Flora/Marine Sphere management: `SSummonOverlay` for summon entities
- `PetSubsystem` — Pet taming, feeding, commands, loyalty management
- `HomunculusSubsystem` — Homunculus companion management: skills, feeding, evolution
- `CompanionVisualSubsystem` — Visual actor management for pets, homunculus, mounts
- `CombatStatsSubsystem` — `SCombatStatsWidget` (F8 toggle, Z=12) + `SAdvancedStatsWidget` (element/race/size ATK/DEF)

### Unidentified Items & Item Appraisal
- `bIdentified` field on `FInventoryItem` — equipment drops as unidentified (weapon/armor only)
- Generic name mapping: `ItemType` + `WeaponType`/`EquipSlot` to 15+ weapon types + 7 armor types
- Unidentified visuals: orange "?" overlay in inventory, generic tooltip, hidden inspect details
- `SIdentifyPopup` (Z=24) — Item Appraisal UI: modal popup, one item per cast, Magnifier consumable support

### Zone / Map / Warp System
- Multi-zone architecture: zone registry (`ro_zone_data.js`), zone-scoped broadcasting, lazy enemy spawning
- `ZoneTransitionSubsystem` — manages zone:change/error/teleport events, loading overlay, pawn teleport
- `WarpPortal` — overlap trigger actors for zone-to-zone transitions
- `KafraSubsystem` + `SKafraWidget` — Kafra NPC service dialog (save point, teleport with zeny cost)
- `KafraNPC` — clickable NPC actors
- Login loads correct map directly from character REST data (zone_name + level_name)
- Level Blueprint spawns and possesses `BP_MMOCharacter` (DefaultPawnClass = None on GameMode)
- 4 zones: prontera (town), prontera_south (starter field), prontera_north (field), prt_dungeon_01 (dungeon)
- Position persistence: disconnect handler + 60s periodic save + 5s Level Blueprint save
- DB migration: `database/migrations/add_zone_system.sql`

### Map System (Minimap + World Map)
- **Minimap**: `MinimapSubsystem` + `SMinimapWidget` — 128x128 top-right SceneCapture2D overhead view
  - Orthographic camera (256x256 RT, 8 FPS capture, 5000 unit height, OrthoWidth 4000)
  - 5 zoom levels (factors: 1, 1.8, 3, 5, 8), 3 opacity states (Tab cycles: opaque/transparent/hidden)
  - Entity dots: enemies (orange), other players (white), party (pink), NPCs (blue), warps (red), player (white center)
  - Guide NPC marks (blinking crosses), `/where` chat command for coordinates
- **World Map**: `SWorldMapWidget` — fullscreen 12x8 grid overlay on continent illustration
  - 62 unique zones with category tinting (town=gold, field=green, dungeon=red)
  - Hover: gold border + tooltip (name, type, level, monsters), Alt/N=zone names, Tab=monster info
  - Party member cross-zone dots (pink), current zone indicator (white)
  - Keyboard: M=open/close, N=zone names, Tab=monsters, Esc=close
- **Server data**: `ro_world_map_data.js` — 12x8 grid, ZONE_INFO lookup, monster data per zone
- **Socket events**: `map:world_data` (zone:ready), `map:party_positions` (6 zone-change paths), `map:mark` (Guide NPC)

### Status Effect & Buff System (Phase 2)
- **10 generic status effects**: Stun, Freeze, Stone, Sleep, Poison, Blind, Silence, Confusion, Bleeding, Curse
- `ro_status_effects.js` — resistance formulas, apply/remove/cleanse/tick, periodic drains, damage-break mechanics
- `ro_buff_system.js` — generic buff system (50+ types: Provoke, Endure, Sight, Blessing, Increase/Decrease AGI, Angelus, Hiding, Ruwach, Energy Coat, Loud Exclamation, Improve Concentration, Auto Berserk, Signum Crucis, Pneuma, and all second-class buffs), stacking rules, stat modifiers
- `getCombinedModifiers(target)` merges status + buff modifiers for damage/combat calculations
- Server enforcement: movement lock during CC, AI CC lock (enemies can't move/attack), skill/attack prevention, regen blocking
- `checkDamageBreakStatuses()` breaks freeze/stone/sleep/confusion on ANY damage (replaces per-skill fire-breaks-freeze)
- `BuffBarSubsystem` + `SBuffBarWidget` — client Slate UI showing buff/status icons with timers (Z=11)
- Buffs persist naturally across zone transitions (persistent socket, no disconnect/reconnect)
- Socket events: `status:applied`, `status:removed`, `status:tick`, `buff:list`, `buff:request`, `buff:removed`
- Debug commands: `debug:apply_status`, `debug:remove_status`, `debug:list_statuses` (dev only)
- Skills: `/sabrimmo-buff`, `/sabrimmo-debuff`

### Consumable & Scroll Systems
- **ASPD potions**: Concentration Potion, Awakening Potion, Berserk Potion — `sc_start` handler with class-based restrictions (Berserk Potion: Blacksmith/Assassin Cross only) and base level requirements, ASPD buff types with mutual exclusion
- **Scroll system**: `itemskill` scrolls that cast skills on use — bolt scrolls (Cold Bolt, Fire Bolt, Lightning Bolt), heal scrolls, and Elemental Converter consumables that apply endow buffs (Fire/Water/Earth/Wind)
- **Stat food consumables**: 60 stat food items providing +1 to +10 bonuses for each of the 6 base stats (STR/AGI/VIT/INT/DEX/LUK), applied as timed buffs via `sc_start`

### Magic Rod Absorption
- **Magic Rod** (Sage skill 1403): Wired into 8 single-target magic damage paths — absorbs incoming single-target spells, cancels the damage, and converts SP cost into caster's SP recovery
- Integrated in: bolt skills (Fire/Cold/Lightning/Earth Spike/Heaven's Drive), Napalm Beat, Soul Strike, Jupiter Thunder, Holy Light

### Ensemble System (Bard/Dancer)
- **9 ensemble skills** requiring both a Bard and Dancer performing together in overlapping range
- Lullaby (sleep zone), Mr. Kim A Rush Hour (EXP bonus zone), Eternal Chaos (DEF reduction zone), Drum on the Battlefield (ATK/DEF bonus zone), Ring of Nibelungen (weapon Lv4 ATK bonus), Loki's Veil (no-skill zone), Into the Abyss (no-gemstone zone), Invulnerable Siegfried (element + status resist zone), Moonlit Water Mill (weight reduction zone)
- Ground effect tick system with dual-performer SP drain, automatic cancellation when performers separate

### Abracadabra (Sage)
- **145 regular skills** randomly selected and cast at the target — full RO Classic canonical skill pool
- **6 special effects**: Summon Monster, Class Change (normal monsters only), Level Up (job level +1), Enchant Weapon (random endow), random stat +1/+10 effects
- SP cost 50 per cast, Yellow Gemstone catalyst consumed, cast time and cooldown enforced

### Crafting & Production Skills
- **Create Elemental Converter** (Sage 1421): 4 recipes producing Fire/Water/Earth/Wind Elemental Converter consumables from base materials (Scorpion Tail, Crystal Blue, Green Live, Wind of Verdure)
- **Ore Discovery** (Blacksmith passive 1221): On monster kill, 20-item IG_ORE group roll (Iron Ore, Coal, Iron, Steel, Rough Wind, etc.) with flat proc chance per skill level
- **Weapon Repair** (Blacksmith active 1222): Repairs broken equipment on self or party members, material cost based on weapon level (Iron Ore/Iron/Steel/Rough Oridecon/Oridecon), removes `weaponBroken` status

### Resurrection Skills
- **Redemptio** (Priest 1018): Mass party resurrection — revives all dead party members within range, caster loses `(targetCount * 2)%` base EXP as penalty, 0% HP on revive (requires follow-up healing), area-of-effect check within skill range

### Elemental Change (Sage)
- **Elemental Change Fire/Water/Earth/Wind** (Sage skills 1416-1419): Permanently changes a monster's element to the specified type at element level 1, works on normal monsters only (not bosses), consumes elemental catalyst

### Homunculus System (Extended)
- **Homunculus combat**: Enemies can target and attack homunculi — damage reception with FLEE-based dodge, hardDEF/softDEF reduction, death and revival cycle
- **Homunculus skills**: 8 active skills across 4 homunculus types — Lif (Healing Hands heal, Urgent Escape flee buff), Amistr (Castling position swap, Amistr Bulwark DEF buff), Filir (Moonlight multi-hit attack, Flitting move speed buff), Vanilmirth (Caprice random element attack, Chaotic Blessing random heal/damage)
- **Homunculus evolution**: Stone of Sage consumable + Loyal intimacy threshold triggers evolution — stat bonuses applied, evolved form unlocked, 4th skill slot becomes available
- Homunculus persistence: full state saved to `character_homunculus` DB table (HP/SP/EXP/stats/intimacy/hunger/skills/evolved status)

### Monster Skill System (Extended)
- **NPC_SUMMONSLAVE**: Boss monsters spawn slave minions on HP thresholds or timers — slave lifecycle tied to master (slaves despawn when master dies), configurable slave types and counts per boss template
- **NPC_METAMORPHOSIS**: Egg/larval form transformation — monsters change into a different template at HP thresholds (e.g., Pupa -> Creamy), full stat recalculation on transform, death of original form triggers new form spawn
- **40+ NPC_ monster skills** across 12+ configured monsters in `ro_monster_skills.js`
- 7 execution functions: targetCast, selfBuff, aoeAttack, debuffAttack, summonSlave, metamorphosis, healing

### Automated UI Testing
- `ASabriMMOUITests` — C++ test runner for automated UI validation
- `BP_AutomationTestLibrary` — Blueprint function library for UI testing
- Tests cover: GameInstance, PlayerCharacter, Inventory, Zuzucoin updates
- Auto-executes on BeginPlay with 5-second delay, results in Output Log and on-screen
- Integration with UE5 Automation system for CI/CD pipeline support

## Project Directory Structure

```
C:/Sabri_MMO/
├── client/SabriMMO/              # UE5 project
│   ├── Source/SabriMMO/          # C++ source (19 core files + 66 UI files + 6 VFX files + 76 variant files)
│   │   ├── CharacterData.h       # FCharacterData struct
│   │   ├── MMOGameInstance.*     # Auth state, character list, events
│   │   ├── MMOHttpManager.*      # REST API client (BlueprintFunctionLibrary)
│   │   ├── SabriMMOCharacter.*   # Base player character
│   │   ├── SabriMMOPlayerController.*  # Input mapping
│   │   ├── SabriMMOGameMode.*    # Game mode stub
│   │   ├── OtherCharacterMovementComponent.*  # Remote player movement
│   │   ├── SabriMMO.*            # Module definition + log category
│   │   ├── SabriMMO.Build.cs     # Build config (17 module dependencies)
│   │   ├── UI/                    # 33 subsystems + 30+ Slate widgets (66 files)
│   │   │   ├── LoginFlowSubsystem.*  # Login flow state machine (replaces BP_GameFlow)
│   │   │   ├── SLoginWidget.*      # Login screen
│   │   │   ├── SServerSelectWidget.* # Server selection screen
│   │   │   ├── SCharacterSelectWidget.* # Character selection + delete confirmation
│   │   │   ├── SCharacterCreateWidget.* # Character creation screen
│   │   │   ├── SLoadingOverlayWidget.*  # "Please Wait" overlay
│   │   │   ├── SBasicInfoWidget.*  # Draggable Slate HUD panel
│   │   │   ├── BasicInfoSubsystem.* # UWorldSubsystem for data + socket wrapping
│   │   │   └── ... (12 more Slate widgets + subsystems)
│   │   ├── Variant_Combat/       # Combat system (42 files)
│   │   ├── Variant_Platforming/  # Platforming variant (8 files)
│   │   └── Variant_SideScrolling/ # Side-scrolling variant (26 files)
│   ├── Content/SabriMMO/         # Blueprint assets (.uasset — not in git)
│   └── SabriMMO.uproject         # UE5 5.7 project file
├── server/
│   ├── src/index.js              # Monolithic server (~32,200 lines)
│   ├── src/ro_*.js               # 12 data modules (~6,000+ lines, includes ro_navmesh.js)
│   ├── package.json              # 10 dependencies
│   ├── .env                      # DB credentials, JWT secret
│   └── logs/                     # Runtime logs
├── database/
│   ├── init.sql                  # Schema: users, characters, items, character_inventory, character_hotbar
│   ├── migrations/                # 25 migration files (see INDEX.md for full list)
│   ├── create_test_users.*       # Test user scripts
│   └── insert_test_user.sql
├── scripts/                      # Utility scripts
│   ├── extract_ro_monsters_v2.js # RO monster extraction (rAthena → JS)
│   └── generate_ro_items_migration.js # RO items SQL + mapping generator
├── docs/                         # Legacy documentation (31 files)
├── docsNew/                      # This comprehensive documentation
└── .gitignore
```

## UE5 Plugin Dependencies

| Plugin | Purpose |
|--------|---------|
| **ModelingToolsEditorMode** | In-editor modeling |
| **StateTree** | AI behavior trees |
| **GameplayStateTree** | Gameplay-specific state trees |
| **VisualStudioTools** | VS integration (Win64) |
| **Diversion** | Version control plugin |
| **RemoteControl** | Remote control API |

## C++ Module Dependencies (SabriMMO.Build.cs)

```
Core, CoreUObject, Engine, InputCore, EnhancedInput,
AIModule, StateTreeModule, GameplayStateTreeModule,
UMG, Slate, HTTP, Json, JsonUtilities
```

## Server Dependencies (package.json)

| Package | Version | Purpose |
|---------|---------|---------|
| express | ^4.18.2 | REST API framework |
| socket.io | ^4.8.3 | Real-time WebSocket |
| pg | ^8.11.3 | PostgreSQL client |
| redis | ^5.10.0 | Redis cache client |
| bcrypt | ^5.1.1 | Password hashing |
| jsonwebtoken | ^9.0.3 | JWT authentication |
| cors | ^2.8.5 | Cross-origin requests |
| express-rate-limit | ^8.2.1 | API rate limiting |
| dotenv | ^16.3.1 | Environment variables |
| recast-navigation | ^0.42.1 | NavMesh pathfinding (Recast/Detour WASM) |
| nodemon | ^3.0.2 | Dev auto-restart |

## Current Status

**Last Updated**: 2026-03-20

- **Completed**: Foundation, Multiplayer, Combat, Stats, Inventory, Equipment, Hotbar, NPC Shops, Zone System, Skill VFX, Status Effect & Buff System (Phase 2), Element Table & Formula Audit (Phase 3), Persistent Socket Connection (Phase 4), Passive Skills & First Class Completion (Phase 5), Dual Wield System (Assassin), Blueprint-to-C++ Migration Phase 1 (Camera+Input) + Phase 2 (Targeting+Combat), Second Class Foundation (Phase 0: data fixes + ground effects + mount system), Second Class Phase 1 (Assassin+Priest+Knight), Phase 2 (Crusader+Wizard+Sage), Phase 3 (Monk+Hunter), Phase 4 (Bard+Dancer), Phase 5 (Blacksmith+Rogue+Forging/Refining), Phase 6 (Alchemist+Homunculus), Deferred Systems Remediation (Magic Rod, Ensembles, ASPD Potions, Scrolls, Stat Foods, Ore Discovery, Weapon Repair, Abracadabra, Elemental Converters, Elemental Change, Redemptio, Homunculus Combat/Skills/Evolution, Monster Summoning/Metamorphosis), Merchant UI Systems (Cart Inventory, Vending Setup/Browse with vendor self-view + live sale updates + movement lock + buyer quantity input + click-to-browse, Item Appraisal, Unidentified Items)
- **Monsters**: 509 RO templates loaded, 46 spawns active (zones 1-3), full AI state machine with CC lock + hidden player detection, monster skill system (40+ NPC_ skills, summoning, metamorphosis), enemy sprite system (SpriteCharacterActor with walk/attack/hit/death animations, spriteClass/weaponMode per template), NavMesh pathfinding (recast-navigation v0.42.1, OBJ export from UE5, binary cache, de-aggro system)
- **Items**: 6,169 rAthena canonical items + 538 cards + 60 stat foods + ASPD potions + scrolls + elemental converters in database
- **Skills**: 69 first-class + 224 second-class = 293 skill definitions, 180+ active skill handlers, 33+ passive skills, 95 buff types, 10 status effects, 97 VFX configs, 8 homunculus skills, Abracadabra (145 skills + 6 special effects)
- **Classes**: All 6 first classes fully playable + 13 second classes: Assassin, Priest, Knight, Crusader, Wizard, Sage, Hunter, Bard, Dancer, Monk, Rogue, Blacksmith, Alchemist
- **Combat Data**: Element table (10×10×4 = 400 values) verified against rAthena pre-renewal `attr_fix.yml`, size penalty table (18 weapons × 3 sizes) verified, card modifier stacking fixed (per-category multiplicative), card compound system with `rebuildCardBonuses()` integration, race ATK/DEF passive bonuses (Demon Bane, Divine Protection), dual wield per-hand card/element for auto-attacks, Lex Aeterna consumption in 8 damage paths, weapon element override via buff system (Aspersio/Enchant Poison), Magic Rod absorption in 8 single-target magic paths
- **Dual Wield**: Assassin/Assassin Cross dual wield (8 phases complete). Both hands hit per cycle, per-hand mastery penalties, per-hand cards/elements, ASPD combined formula, Katar/DW mutual exclusivity, combat stats display
- **UI**: 34 C++ Slate subsystems + 30+ Slate widgets (all Blueprint widgets replaced), BuffBar with timer icons, Cart/Vending/Identify merchant UIs, Party UI, Crafting popup, Summon overlay, Pet/Homunculus management, Advanced Stats panel, Minimap (SceneCapture2D, 5 zoom, 3 opacity, entity dots), World Map (12x8 grid, hover tooltips, party dots)
- **Zones**: 4 zones (prontera, prt_south, prt_north, prt_dungeon_01)
- **Homunculus**: Companion system for Alchemist class — 4 homunculus types with growth tables, auto-attack in combat tick, EXP sharing, hunger/intimacy system, full DB persistence, evolution via Stone of Sage, 8 active skills across 4 types, enemies can target and damage homunculi
- **Consumables**: ASPD potions (3 tiers with class restrictions), stat food (+1 to +10 per stat), itemskill scrolls (bolt/heal), elemental converter endow items
- **Ensemble**: 9 Bard/Dancer ensemble skills (Lullaby, Mr. Kim, Eternal Chaos, Drum, Nibelungen, Loki's Veil, Into the Abyss, Siegfried, Moonlit Water Mill)
- **Deferred Systems Remediation**: ALL 38 audit items fixed, zero remaining (10/10 phases complete)
- **Server**: 32,200 lines in `index.js` + 11 data modules (~6,000 lines), 79 socket event handlers, 11 REST endpoints
- **Next**: Client-side homunculus actor, homunculus position broadcast, PvP/WoE systems
- **Roadmap**: See [Strategic_Implementation_Plan_v3.md](05_Development/Strategic_Implementation_Plan_v3.md)

---

**Last Updated**: 2026-03-20 — Merchant UI Systems: Cart Inventory (CartSubsystem, F10, drag-drop), Vending (VendingSubsystem, setup/browse popups, shop sign, vendor self-view with live sale log, movement lock, buyer quantity input, click-to-browse via PlayerInputSubsystem), Item Appraisal (SIdentifyPopup, one-per-cast), Unidentified Items (bIdentified, generic names, orange "?" overlay)
**Engine**: Unreal Engine 5.7
**Server**: Node.js 18+ LTS
