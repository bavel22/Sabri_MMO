# Sabri_MMO — Project Overview

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
- RO-style wandering AI (random movement within spawn radius)
- Server-side health tracking, death, and timed respawn
- **126 RO drop items** integrated with inventory system (consumables, weapons, armor, cards, etc.)
- **15 existing game items** added as extra drops on appropriate monsters
- Drop tables with chance-based loot rolling using pre-resolved itemIds

### Items & Inventory
- **148 total items**: 22 original + 126 RO items from monster drops
  - **28 consumables** (5 original + 23 RO): Herbs, fruits, potions, scrolls
  - **58 etc items** (8 original + 50 RO): Materials, gems, dolls, ammo
  - **20 weapons** (6 original + 14 RO): Daggers, swords, maces, staves, bows, spears, axes, instruments
  - **14 armor** (3 original + 11 RO): Body armor, shields, headgear, accessories
  - **23 monster cards**: Stat bonuses and special effects
- PostgreSQL `items` (definitions) + `character_inventory` (per-character)
- Socket.io events: load, use consumable, equip/unequip, drop/discard
- Equipped weapon modifies ATK, range, and ASPD
- Stackable items with max stack limits
- **RO item name → ID mapping** for efficient drop processing

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

### UI System — 18 Blueprint Widgets + 14 Slate Widgets

#### Login Flow (Pure C++ Slate — replaces BP_GameFlow)
- `ULoginFlowSubsystem` — UWorldSubsystem state machine: Login → ServerSelect → CharacterSelect → CharacterCreate → EnteringWorld
- `SLoginWidget` — Username/password, remember username, error display, Enter/Tab keyboard shortcuts
- `SServerSelectWidget` — Scrollable server list with population/status, selection highlighting
- `SCharacterSelectWidget` — 3x3 card grid + detail panel (HP/SP bars, 6 stats, EXP), delete confirmation with password
- `SCharacterCreateWidget` — Name field, gender toggle, hair style/color pickers (arrow buttons), locked Novice class
- `SLoadingOverlayWidget` — Dimmed fullscreen overlay with "Please Wait" dialog and progress bar

#### Game HUD (Blueprint)
- `WBP_GameHUD` — HP/MP bars, target frame
- `WBP_PlayerNameTag` — Floating name above characters
- `WBP_ChatWidget` / `WBP_ChatMessageLine` — Chat interface
- `WBP_StatAllocation` — Stat point distribution
- `WBP_TargetHealthBar` — World-space health bar on enemies/players
- `WBP_InventoryWindow` / `WBP_InventorySlot` — Inventory grid with icons, double-click, right-click, drag-and-drop
- `WBP_ContextMenu` — Right-click context menu (Equip/Use/Drop)
- `WBP_DragVisual` — Ghost widget during inventory drag
- `WBP_ItemTooltip` — Hover tooltip for inventory slots
- `WBP_DeathOverlay` — Death screen with respawn button
- `WBP_LootPopup` — Loot notification popup with auto-fade

#### Game HUD (Pure C++ Slate — 15 UWorldSubsystems)
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
- `SCastBarOverlay` — World-projected cast time bars (Z=25)
- `SWorldHealthBarOverlay` — Floating HP/SP bars above characters (Z=8)

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

### Status Effect & Buff System (Phase 2)
- **10 generic status effects**: Stun, Freeze, Stone, Sleep, Poison, Blind, Silence, Confusion, Bleeding, Curse
- `ro_status_effects.js` — resistance formulas, apply/remove/cleanse/tick, periodic drains, damage-break mechanics
- `ro_buff_system.js` — generic buff system (Provoke, Endure, Sight + 20 future types), stacking rules, stat modifiers
- `getCombinedModifiers(target)` merges status + buff modifiers for damage/combat calculations
- Server enforcement: movement lock during CC, AI CC lock (enemies can't move/attack), skill/attack prevention, regen blocking
- `checkDamageBreakStatuses()` breaks freeze/stone/sleep/confusion on ANY damage (replaces per-skill fire-breaks-freeze)
- `BuffBarSubsystem` + `SBuffBarWidget` — client Slate UI showing buff/status icons with timers (Z=11)
- `reconnectBuffCache` — preserves buffs across zone change disconnect/reconnect (30s TTL)
- Socket events: `status:applied`, `status:removed`, `status:tick`, `buff:list`, `buff:request`, `buff:removed`
- Debug commands: `debug:apply_status`, `debug:remove_status`, `debug:list_statuses` (dev only)
- Skills: `/sabrimmo-buff`, `/sabrimmo-debuff`

### Automated UI Testing
- `ASabriMMOUITests` — C++ test runner for automated UI validation
- `BP_AutomationTestLibrary` — Blueprint function library for UI testing
- Tests cover: GameInstance, PlayerCharacter, HUDManager, Inventory, Zuzucoin updates
- Auto-executes on BeginPlay with 5-second delay, results in Output Log and on-screen
- Integration with UE5 Automation system for CI/CD pipeline support

## Project Directory Structure

```
C:/Sabri_MMO/
├── client/SabriMMO/              # UE5 project
│   ├── Source/SabriMMO/          # C++ source (23 core files + 76 variant files)
│   │   ├── CharacterData.h       # FCharacterData struct
│   │   ├── MMOGameInstance.*     # Auth state, character list, events
│   │   ├── MMOHttpManager.*      # REST API client (BlueprintFunctionLibrary)
│   │   ├── SabriMMOCharacter.*   # Base player character
│   │   ├── SabriMMOPlayerController.*  # Input mapping
│   │   ├── SabriMMOGameMode.*    # Game mode stub
│   │   ├── OtherCharacterMovementComponent.*  # Remote player movement
│   │   ├── SabriMMO.*            # Module definition + log category
│   │   ├── SabriMMO.Build.cs     # Build config (17 module dependencies)
│   │   ├── UI/                    # Slate UI subsystems + widgets (30+ files)
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
│   ├── src/index.js              # Monolithic server (~8500 lines)
│   ├── package.json              # 10 dependencies
│   ├── .env                      # DB credentials, JWT secret
│   └── logs/                     # Runtime logs
├── database/
│   ├── init.sql                  # Schema: users, characters, items, character_inventory
│   ├── migrations/                # Database migrations
│   │   ├── add_ro_drop_items.sql   # 126 RO drop items migration
│   │   ├── add_character_customization.sql  # hair_style, hair_color, gender, delete_date
│   │   ├── add_soft_delete.sql        # Soft-delete flag (deleted BOOLEAN) for characters
│   │   ├── add_equipped_position.sql  # Dual-accessory support
│   │   └── add_hotbar_multirow.sql    # 4-row hotbar storage
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
| nodemon | ^3.0.2 | Dev auto-restart |

## Current Status

**Last Updated**: 2026-03-10

- **Completed**: Foundation, Multiplayer, Combat, Stats, Inventory, Equipment, Hotbar, NPC Shops, Zone System, Skill VFX, Status Effect & Buff System (Phase 2), Element Table & Formula Audit (Phase 3)
- **In Progress**: Persistent Socket Connection (planning stage — see `docsNew/05_Development/Persistent_Socket_Connection_Plan.md`)
- **Monsters**: 509 RO templates loaded, 46 spawns active (zones 1-3), full AI state machine with CC lock
- **Items**: 148 items in database (22 original + 126 RO drops)
- **Skills**: 17 active skills with VFX, 7 passive skill stubs, 10 status effects, 3 active buffs
- **Combat Data**: Element table (10×10×4 = 400 values) verified against rAthena pre-renewal `attr_fix.yml`, size penalty table (18 weapons × 3 sizes) verified, card modifier stacking fixed (per-category multiplicative)
- **UI**: 15 C++ Slate subsystems + 18 Blueprint widgets, BuffBar with timer icons
- **Zones**: 4 zones (prontera, prt_south, prt_north, prt_dungeon_01)
- **Next**: Phase 4 (Persistent Socket Connection) per Strategic Plan v3
- **Roadmap**: See `docsNew/05_Development/Strategic_Implementation_Plan_v3.md`

---

**Last Updated**: 2026-02-24 — Added 126 RO items, disabled zones 4-9, integrated existing items as extra drops
**Engine**: Unreal Engine 5.7
**Server**: Node.js 18+ LTS
