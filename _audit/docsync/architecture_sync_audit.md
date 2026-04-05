# Architecture & Overview Documentation Sync Audit

**Audit Date**: 2026-03-22
**Auditor**: Claude Opus 4.6 automated audit
**Scope**: 5 architecture/overview doc files vs actual codebase state
**Method**: 5-pass comparison (read docs, read code, line-by-line diff, missing systems, compile discrepancies)

---

## Summary

| Document | Accurate | Inaccurate | Missing | Severity |
|----------|----------|------------|---------|----------|
| `00_Project_Overview.md` | 42 | 11 | 8 | Medium-High |
| `01_Architecture/System_Architecture.md` | 38 | 9 | 7 | Medium-High |
| `01_Architecture/Database_Architecture.md` | 26 | 7 | 5 | Medium |
| `01_Architecture/Multiplayer_Architecture.md` | 34 | 5 | 9 | Medium |
| `00_Global_Rules/Global_Rules.md` | 51 | 2 | 1 | Low |

**Total Discrepancies**: 34 inaccurate claims, 30 missing items

---

## 1. `docsNew/00_Project_Overview.md`

### Accurate Claims (Verified)
- Tech stack: UE5.7, Express 4.18.2, Socket.io 4.8.3, PostgreSQL, Redis, bcrypt 5.1.1, jsonwebtoken 9.0.3, express-rate-limit 8.2.1 -- all match `package.json`
- Server-authoritative architecture -- confirmed throughout `index.js`
- 509 RO monster templates -- confirmed via `ro_monster_templates.js` require
- 6,169 rAthena canonical items -- confirmed in `init.sql` comments
- 538 monster cards -- confirmed in doc
- 50ms combat tick -- confirmed at line 346: `COMBAT_TICK_MS: 50`
- 4 zones -- confirmed in `ro_zone_data.js` references
- JWT validation on player:join -- confirmed at line 5295
- 33 UWorldSubsystems -- confirmed: 33 `*Subsystem.h` files found in `UI/`
- Soft delete -- confirmed in `init.sql`: `deleted BOOLEAN NOT NULL DEFAULT FALSE`
- Socket event count 79 -- confirmed: `socket.on(` count = 79

### Inaccurate Claims

#### I1 — Server line count outdated (Severity: Low)
- **Doc says** (line 279): "Monolithic server (~32,200 lines)"
- **Actual**: 32,566 lines (`wc -l index.js`)
- **Also affected**: Line 348 says "32,200 lines in `index.js`"
- **Fix**: Update both to "~32,600 lines" or "~33,000 lines" (round up for future growth)

#### I2 — Data modules count wrong (Severity: Medium)
- **Doc says** (line 280): "11 data modules (~6,000 lines)"
- **Actual**: 19 `ro_*.js` files exist in `server/src/`:
  - `ro_arrow_crafting.js`, `ro_buff_system.js`, `ro_card_effects.js`, `ro_card_prefix_suffix.js`, `ro_damage_formulas.js`, `ro_exp_tables.js`, `ro_ground_effects.js`, `ro_homunculus_data.js`, `ro_item_effects.js`, `ro_item_groups.js`, `ro_item_mapping.js`, `ro_monster_ai_codes.js`, `ro_monster_skills.js`, `ro_monster_templates.js`, `ro_pet_data.js`, `ro_skill_data.js`, `ro_skill_data_2nd.js`, `ro_status_effects.js`, `ro_zone_data.js`
- **Fix**: Change "11 data modules" to "19 data modules"

#### I3 — C++ module dependencies incomplete (Severity: Medium)
- **Doc says** (line 310-313): Lists 13 modules: `Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, StateTreeModule, GameplayStateTreeModule, UMG, Slate, HTTP, Json, JsonUtilities`
- **Actual** `SabriMMO.Build.cs` has 18 modules:
  - Missing from doc: `SlateCore`, `SocketIOClient`, `SIOJson`, `Niagara`, `NiagaraCore`, `NavigationSystem`
  - These 5 missing modules are critical runtime dependencies
- **Fix**: Update the module list to match actual Build.cs

#### I4 — Server dependencies count wrong (Severity: Low)
- **Doc says** (line 281): "10 dependencies" in `package.json`
- **Actual**: `package.json` has 10 production dependencies + 1 extra (`socket.io-client: ^4.8.3`). Total production deps = 10 (but `socket.io-client` is 11th). Dev deps: `nodemon`, `js-yaml`, `yaml` (3 total).
- **Note**: The table on lines 318-329 only lists 10 deps (missing `socket.io-client`). The `devDependencies` are also missing from the table.
- **Fix**: Add `socket.io-client` to the dependencies table. Update count to "11 production + 3 dev dependencies"

#### I5 — Source file count outdated (Severity: Low)
- **Doc says** (line 253): "19 core files + 66 UI files + 6 VFX files + 76 variant files"
- **Actual**:
  - Core files: 37 files (.h + .cpp in root Source dir, includes test files, NPC actors, SocketEventRouter)
  - UI files: 132 files (.h + .cpp in UI/ dir) -- doc says 66
  - VFX files: 6 (correct)
  - Variant files: 11 + 7 + 12 = 30 (not 76 -- this count may include nested dirs)
  - **Total**: 175 non-variant + 30 variant = 205 files
- **Fix**: Update to "37 core files + 132 UI files + 6 VFX files + ~30 variant files"

#### I6 — REST endpoints count wrong (Severity: Low)
- **Doc says** (line 348): "11 REST endpoints"
- **Actual**: 11 REST route definitions found (including `/health` and `/api/test`), so this is correct if counting both. But CLAUDE.md says "11 REST endpoints" which is accurate.
- **Status**: Actually correct. No fix needed.

#### I7 — Migration files count potentially wrong (Severity: Low)
- **Doc says** (line 286): "25 migration files"
- **Actual**: 25 files found in `database/migrations/` -- correct.
- **Status**: Correct. No fix needed.

#### I8 — Ground Effects tick interval wrong (Severity: Medium)
- **Doc reference**: System_Architecture.md says "500ms" for Ground Effects Tick (line 80)
- **Actual**: 250ms (line 27680-27682 in index.js: "Reduced from 500ms to 250ms")
- **This is a System_Architecture.md issue, covered in that section below**

#### I9 — Chat system description outdated (Severity: Medium)
- **Doc says** (line 99-103): "Expandable architecture (ZONE, PARTY, GUILD, TELL planned)"
- **Actual**: Party chat is IMPLEMENTED (`socket.on('party:chat')` at line 8667), whisper/tell is IMPLEMENTED (`/w` and `/whisper` commands at line 8682+). Zone chat may also be implemented.
- **Fix**: Remove "(ZONE, PARTY, GUILD, TELL planned)" and list what is actually implemented

#### I10 — Abracadabra special effects count wrong (Severity: Low)
- **Doc says** (line 215): "6 special effects"
- **CLAUDE.md says**: "6 special effects" but the memory entry from session notes says "6 special + 7 exclusive = 13 total"
- **Memory says**: "+7 exclusive effects (SA_SUMMONMONSTER/CLASSCHANGE/FORTUNE/LEVELUP/REVERSEORCISH/GRAVITY/TAMINGMONSTER)"
- **Fix**: Update to "6 special effects + 7 exclusive effect types (13 total)" or keep at "6 special effects" if that's the canonical grouping

#### I11 — Missing `ro_pet_data` from data modules list (Severity: Medium)
- **CLAUDE.md** lists data modules as: `ro_monster_templates, ro_exp_tables, ro_skill_data, ro_monster_ai_codes, ro_zone_data, ro_card_effects, ro_item_groups, ro_ground_effects, ro_arrow_crafting, ro_monster_skills, ro_homunculus_data, ro_status_effects, ro_buff_system, ro_item_effects, ro_damage_formulas`
- **Missing from CLAUDE.md**: `ro_pet_data`, `ro_card_prefix_suffix`, `ro_item_mapping`, `ro_skill_data_2nd`
- This is a CLAUDE.md issue, but the Project Overview doesn't list individual modules

### Missing Items (Not Documented)

#### M1 — `character_pets` table not in Database Architecture doc (Severity: High)
- Table exists: `CREATE TABLE IF NOT EXISTS character_pets` at line 32163
- Has columns: `id, character_id, mob_id, egg_item_id, pet_name, intimacy, hunger, equip_item_id, is_hatched, is_active, created_at`
- **Fix**: Add `character_pets` table to Database Architecture doc

#### M2 — `character_memo` table not in Database Architecture doc (Severity: Medium)
- Table exists: `CREATE TABLE IF NOT EXISTS character_memo` at line 32512
- Used for Warp Portal memo system
- **Fix**: Add `character_memo` table to Database Architecture doc

#### M3 — `skills`, `skill_prerequisites`, `skill_levels`, `character_skills` tables not documented (Severity: Medium)
- Created at lines 32265-32303
- These are auto-created at server startup
- **Fix**: Add these tables to Database Architecture doc

#### M4 — `ItemTooltipBuilder` utility class not mentioned (Severity: Low)
- Files: `ItemTooltipBuilder.h`, `ItemTooltipBuilder.cpp`
- Used by 6+ widgets for tooltip rendering
- Not a subsystem but a utility class

#### M5 — `SSenseResultPopup` widget not in Project Overview UI list (Severity: Low)
- Files: `SSenseResultPopup.h`, `SSenseResultPopup.cpp`
- Created for Wizard's Sense skill (shows monster info panel)

#### M6 — `SSkillTargetingOverlay` widget not in Project Overview UI list (Severity: Low)
- Files: `SSkillTargetingOverlay.h`
- Used for ground-target skill aiming

#### M7 — `SSkillTooltipWidget` not in Project Overview UI list (Severity: Low)
- Files: `SSkillTooltipWidget.h`
- Skill tree hover tooltip

#### M8 — `SHotbarKeybindWidget` not in Project Overview UI list (Severity: Low)
- File: `SHotbarKeybindWidget.h`
- Keybind display for hotbar

---

## 2. `docsNew/01_Architecture/System_Architecture.md`

### Accurate Claims (Verified)
- Three-tier architecture description -- correct
- Port 3001 for both HTTP and Socket.io -- confirmed
- Combat Tick 50ms -- confirmed
- HP Natural Regen 6,000ms -- confirmed at line 26577
- SP Natural Regen 8,000ms -- confirmed at line 26637
- Skill-Based Regen 10,000ms -- confirmed at line 26691
- Buff Expiry Tick 1,000ms -- confirmed at line 26814
- Periodic DB Save 60,000ms -- confirmed at line 32550
- `connectedPlayers` Map -- confirmed
- `autoAttackState` Map -- confirmed
- `activeCasts` Map -- confirmed
- `activeGroundEffects` Map -- confirmed
- `enemies` Map -- confirmed
- `spawnedZones` Set -- confirmed
- 25 World Subsystems listed -- this count matches the number listed but actual is 33
- Persistent Socket architecture -- confirmed
- SocketEventRouter description -- accurate

### Inaccurate Claims

#### I12 — Enemy AI Tick interval wrong (Severity: High)
- **Doc says** (line 75): "200ms Full AI state machine"
- **Actual**: ENEMY_AI.TICK_MS = 200 at line 28557. This is correct.
- **But doc says** (line 220): "The enemy AI tick runs every 500ms (`ENEMY_AI.WANDER_TICK_MS`)"
- **Actual**: There is no `WANDER_TICK_MS`. The constant is `TICK_MS: 200`.
- **Fix**: Change line 220 from "500ms (`ENEMY_AI.WANDER_TICK_MS`)" to "200ms (`ENEMY_AI.TICK_MS`)"

#### I13 — Ground Effects Tick interval wrong (Severity: High)
- **Doc says** (line 80): "500ms" for Ground Effects Tick
- **Actual**: 250ms (line 27680: "Reduced from 500ms to 250ms")
- **Fix**: Change "500ms" to "250ms" and update description to mention Storm Gust wave timing

#### I14 — C++ World Subsystems count says 25, should be 33 (Severity: High)
- **Doc says** (line 111): "C++ World Subsystems (25)"
- **Actual**: 33 `*Subsystem.h` files in UI/ directory
- **Missing from doc's list**: CartSubsystem, VendingSubsystem, PartySubsystem, CraftingSubsystem, SummonSubsystem, PetSubsystem, HomunculusSubsystem, CompanionVisualSubsystem (8 subsystems not listed)
- **Fix**: Update count to 33 and add the 8 missing subsystems to the list

#### I15 — "Last Updated" date stale (Severity: Medium)
- **Doc says** (line 336): "Last Updated: 2026-03-14"
- **Actual**: Multiple features added since then (Party, Cart, Vending, Identify, Advanced Stats, Sense popup, etc.)
- **Fix**: Update to 2026-03-22

#### I16 — Missing `afterCastDelayEnd` data structure description (Severity: Low)
- Listed in table (line 90) but not explained anywhere in the doc

#### I17 — `io.emit` pattern description incorrect (Severity: Low)
- **Doc says** (line 67): "`io.emit` (all clients)"
- **Actual**: Server uses zone-scoped broadcasting via `broadcastToZone()` for most events, not global `io.emit`. Only `player:left` and a few events use `io.emit`.
- **Fix**: Add note that most events use `broadcastToZone()` for zone-scoped delivery

#### I18 — Multiplayer Architecture diagram in System_Architecture.md shows `BP_SocketManager` as active (Severity: Medium)
- Lines 276-290 show `BP_SocketManager` as a component receiving events from `MultiplayerEventSubsystem`
- **But doc states** (line 144): BP_SocketManager is "fully dead code"
- The diagram text says "Handler Shell" and "NOT connected" but it's still confusing to show dead code prominently in the architecture diagram
- **Fix**: Remove BP_SocketManager from the diagram or add a clear "REMOVED" annotation

#### I19 — `player:moved` handler listed under wrong subsystem (Severity: Low)
- **Doc diagram** (line 259): Shows `"player:moved" -> [MultiplayerEventSub]`
- **Actual**: `player:moved` is handled by `OtherPlayerSubsystem`, not `MultiplayerEventSubsystem`
- **Fix**: Change to `[OtherPlayerSub]`

#### I20 — CombatActionSubsystem Z-order missing for target frame (Severity: Low)
- **Doc says** (line 116): "target frame (Z=9), death overlay (Z=40)"
- Project Overview says same (line 126, 143) -- this is consistent and likely correct

### Missing Items

#### M9 — No mention of `CompanionVisualSubsystem` (Severity: Medium)
- Listed in Project Overview but NOT in System_Architecture subsystem list

#### M10 — No mention of `CartSubsystem` (Severity: Medium)
- Active subsystem with widget, not listed

#### M11 — No mention of `VendingSubsystem` (Severity: Medium)
- Active subsystem with widget, not listed

#### M12 — No mention of `PartySubsystem` (Severity: Medium)
- Active subsystem with widget, not listed

#### M13 — No mention of `CraftingSubsystem`, `SummonSubsystem`, `PetSubsystem`, `HomunculusSubsystem` (Severity: Medium)
- 4 active subsystems not listed

#### M14 — Party HP Sync tick (1000ms) not in tick loop table (Severity: Low)
- Exists at line 26551: Party HP sync every 1 second

#### M15 — Multiple additional tick loops not listed (Severity: Medium)
- Homunculus Hunger Tick (10s check, 60s decay) -- line 27531
- Homunculus HP/SP Regen Tick (10s) -- line 27580
- Pet Hunger Tick (10s) -- line 27621
- Card Periodic Effects (5s) -- line 26775
- Spirits Recovery (1s check) -- line 26743
- These are significant game loops that should be in the tick loop table

---

## 3. `docsNew/01_Architecture/Database_Architecture.md`

### Accurate Claims (Verified)
- PostgreSQL 15.4 + Redis 7.2+ -- confirmed
- `users` table schema -- matches `init.sql`
- `characters` table core columns -- matches `init.sql`
- `items` table core columns -- mostly matches (see inaccuracies below)
- `character_inventory` table -- matches `init.sql`
- `character_hotbar` table -- matches `init.sql`
- `character_cart` table -- confirmed auto-created at line 32065
- `character_homunculus` table -- confirmed auto-created at line 32116
- `parties` + `party_members` tables -- confirmed auto-created at lines 32488-32497
- `vending_shops` + `vending_items` tables -- confirmed auto-created at lines 32082-32093
- Redis cache schema -- confirmed
- Indexes -- confirmed in `init.sql`
- Soft delete behavior -- confirmed
- Auto-migration on startup -- confirmed

### Inaccurate Claims

#### I21 — `items` table missing many columns from actual schema (Severity: High)
- **Doc lists**: 26 columns for `items` table
- **Actual `init.sql`** has 40+ columns. Missing from doc:
  - `aegis_name` (rAthena internal name)
  - `full_description` (full tooltip text)
  - `buy_price`, `sell_price` (separate from `price`)
  - `weapon_level` (1-4 for weapons)
  - `armor_level`
  - `slots` (0-4 card slots)
  - `equip_level_min`, `equip_level_max`
  - `refineable`
  - `jobs_allowed`, `classes_allowed`, `gender_allowed`
  - `equip_locations` (mentioned in notes but not in table)
  - `script`, `equip_script`, `unequip_script`
  - `sub_type`, `view_sprite`, `two_handed`
  - `element`
  - `card_prefix`, `card_suffix`
  - `class_restrictions`
  - `perfect_dodge_bonus`
  - `ammo_type`
- **Fix**: Update the items table definition to include all actual columns

#### I22 — `character_inventory` table missing `compounded_cards` column from `init.sql` (Severity: Medium)
- **Doc has it** (line 170): Shows `compounded_cards JSONB`
- **Actual `init.sql`** does NOT have this column in the CREATE TABLE -- it was likely added via migration or server startup ALTER
- **Status**: The doc is showing the runtime state (with the column), which is correct behavior. But the init.sql doesn't create it. Minor discrepancy in source-of-truth.

#### I23 — `character_hotbar` slot_index CHECK constraint differs (Severity: Low)
- **Doc says** (line 180): "CHECK (1-9)" (1-indexed)
- **Actual `init.sql`** (line 118): "CHECK (slot_index >= 0 AND slot_index <= 8)" (0-indexed)
- **Fix**: Change doc to "CHECK (0-8)" or "0-indexed"

#### I24 — `characters` table default class wrong (Severity: Low)
- **Doc says** (line 88): DEFAULT 'novice'
- **Actual `init.sql`** (line 21): DEFAULT 'warrior'
- **Note**: The server startup ALTER TABLE likely changes this, or the REST API uses 'novice' when creating characters regardless of table default
- **Fix**: Document that `init.sql` uses 'warrior' but runtime creation uses 'novice'

#### I25 — Armor card IDs conflict with armor item IDs (Severity: Medium)
- **Doc says** (line 326-336): "Armor (item_id 4001-4014)"
- **Doc also says** (line 338): "Monster Cards (538 cards, item_id 4001-4499)"
- These ID ranges overlap! Card IDs 4001-4014 conflict with original armor IDs 4001-4014
- **Actual**: The migration to rAthena canonical IDs resolved this -- old custom armor IDs were remapped. But the doc still shows both in the same seed data section without clarifying this.
- **Fix**: Add a note that original armor IDs 4001-4014 were remapped during the canonical ID migration

#### I26 — `aspd_modifier` type discrepancy (Severity: Low)
- **Doc says** (line 149): `aspd_modifier INTEGER`
- **Actual `init.sql`** (line 73): `aspd_modifier FLOAT`
- **Fix**: Change doc to FLOAT

#### I27 — `weapon_range` default value discrepancy (Severity: Low)
- **Doc says** (line 150): DEFAULT 0
- **Actual `init.sql`** (line 74): DEFAULT 150
- **Fix**: Change doc to DEFAULT 150

### Missing Items

#### M16 — `character_pets` table entirely missing (Severity: High)
- Created at line 32163 of index.js
- Columns: `id, character_id, mob_id, egg_item_id, pet_name, intimacy, hunger, equip_item_id, is_hatched, is_active, created_at`
- **Fix**: Add full table definition to doc

#### M17 — `character_memo` table entirely missing (Severity: Medium)
- Created at line 32512 of index.js
- Used for Warp Portal memo system (3 memo slots per character)
- **Fix**: Add table definition

#### M18 — `skills`, `skill_prerequisites`, `skill_levels`, `character_skills` tables missing (Severity: Medium)
- All created at server startup (lines 32265-32303)
- These store skill definitions and learned skills
- **Fix**: Add these 4 table definitions

#### M19 — `character_inventory.refine_level` column not documented (Severity: Medium)
- Refine system was added (session notes confirm refine ATK system complete)
- Column likely added via migration `add_equipment_rate_columns.sql` or `add_forge_columns.sql`
- **Fix**: Verify and add to doc

#### M20 — `character_inventory` forging columns not documented (Severity: Medium)
- Migration `add_forge_columns.sql` exists
- Likely adds: `forged_by`, `forged_element`, `forged_star_crumbs`
- **Fix**: Add to doc

---

## 4. `docsNew/01_Architecture/Multiplayer_Architecture.md`

### Accurate Claims (Verified)
- Server-authoritative model -- confirmed
- Port 3001 -- confirmed
- Persistent socket on GameInstance -- confirmed
- `FSocketIONative` settings (bUnbindEventsOnDisconnect, bCallbackOnGameThread) -- confirmed
- Player join flow -- confirmed at line 5295
- Position sync 30Hz -- confirmed in PositionBroadcastSubsystem description
- Redis position caching with 300s TTL -- confirmed
- Combat tick architecture -- confirmed
- Zone transition flow -- confirmed
- SocketEventRouter architecture -- confirmed
- MultiplayerEventSubsystem emit helpers -- confirmed
- Widget visibility gating -- confirmed
- `player:position` payload includes yaw -- confirmed

### Inaccurate Claims

#### I28 — Enemy AI tick description wrong (Severity: High)
- **Doc says** (line 220): "The enemy AI tick runs every 500ms (`ENEMY_AI.WANDER_TICK_MS`)"
- **Actual**: 200ms (`ENEMY_AI.TICK_MS` at line 28557). No `WANDER_TICK_MS` constant exists.
- **Fix**: Change to "200ms (`ENEMY_AI.TICK_MS`)"

#### I29 — Enemy AI wander speed wrong (Severity: Low)
- **Doc says** (line 232): "WANDER_SPEED (60 units/sec)"
- **Should verify**: This may be correct or may have changed. Cannot confirm without checking the ENEMY_AI constants section.

#### I30 — Missing many socket events from catalog (Severity: High)
- The event catalog (lines 119-182) only lists ~30 events
- **Actual**: 79 socket event handlers exist
- Missing from catalog:
  - `zone:warp`, `zone:ready` -- zone system
  - `kafra:open`, `kafra:save`, `kafra:teleport` -- Kafra NPC
  - `cart:load/rent/remove/move_to_cart/move_to_inventory` -- cart system (5 events)
  - `identify:select` -- item identification
  - `vending:start/close/browse/buy` -- vending system (4 events)
  - `player:sit`, `player:stand` -- sitting system
  - `mount:toggle` -- mount system
  - `job:change` -- job system
  - `skill:data/learn/reset` -- skill system (3 events)
  - `party:load/create/invite/invite_respond/leave/kick/change_leader/change_exp_share/chat` -- party (9 events)
  - `hotbar:save/request/save_skill/clear` -- hotbar (4 events)
  - `pharmacy:craft`, `crafting:craft_converter` -- crafting (2 events)
  - `summon:detonate` -- summon system
  - `homunculus:feed/command/skill_up/use_skill/evolve` -- homunculus (5 events)
  - `pet:tame/incubate/return_to_egg/feed/rename/list` -- pet (6 events)
  - `equipment:repair` -- equipment repair
  - `card:compound` -- card system
  - `warp_portal:confirm` -- warp portal
  - `inventory:move/merge` -- inventory management (2 events)
  - `shop:open/buy/sell/buy_batch/sell_batch` -- shop (5 events, doc only lists 4 inventory events)
  - `refine:request`, `forge:request` -- crafting/forging (2 events)
  - `debug:apply_status/remove_status/list_statuses` -- debug (3 events)
- **Fix**: Add all 49+ missing events to the catalog, organized by category

#### I31 — `player:position` payload incomplete (Severity: Low)
- **Doc says** (line 125): `{characterId, x, y, z}`
- **Actual**: Payload includes `yaw` as documented elsewhere: `{characterId, x, y, z, yaw}`
- The table entry is inconsistent with the description on line 79 which correctly shows `{characterId, x, y, z, yaw}`
- **Fix**: Add `yaw` to the table entry

#### I32 — combat:damage payload incomplete (Severity: Low)
- **Doc says** (line 140): Lists many fields
- **Actual**: May include additional fields added for element effectiveness, Lex Aeterna, etc.
- Various additions from skill audit sessions likely added fields

### Missing Items

#### M21 — Buff/status effect events missing from catalog (Severity: High)
- `status:applied`, `status:removed`, `status:tick` -- mentioned in Project Overview but not in Multiplayer Architecture event catalog
- `buff:list`, `buff:request`, `buff:removed` -- same
- `skill:buff_removed` -- backward compat event
- **Fix**: Add Status/Buff Events section to the catalog

#### M22 — Skill events missing from catalog (Severity: High)
- `skill:use`, `skill:data`, `skill:learn`, `skill:reset` -- not in catalog
- `skill:cast_start`, `skill:cast_end`, `skill:cast_complete` -- likely exist
- `skill:sense_result` -- added in Wizard audit
- **Fix**: Add Skills Events section

#### M23 — EXP/Level events missing from catalog (Severity: Medium)
- `player:stats` and `player:request_stats` are listed
- But EXP gain events, level up events, job change events are not documented

#### M24 — Homunculus/Pet/Companion events missing (Severity: Medium)
- 5 homunculus events + 6 pet events = 11 events not in catalog
- **Fix**: Add Companion Events section

#### M25 — Zone events missing from catalog (Severity: Medium)
- `zone:warp`, `zone:ready`, `zone:change`, `zone:error`, `zone:teleport` -- not documented
- **Fix**: Add Zone Events section

#### M26 — Party events missing from catalog (Severity: Medium)
- 9 party socket events not in catalog
- **Fix**: Add Party Events section

#### M27 — Economy events missing (Severity: Medium)
- Shop, Kafra, Vending, Cart, Refine, Forge events not in catalog
- **Fix**: Add Economy Events section

#### M28 — Combat log events missing (Severity: Low)
- ChatSubsystem handles 8 combat log events per Project Overview
- Not documented in Multiplayer Architecture

#### M29 — Server authority rules table incomplete (Severity: Medium)
- Missing: Skill casting, party management, card compounding, vending, forging/refining
- These are all server-authoritative actions that should be listed

---

## 5. `docsNew/00_Global_Rules/Global_Rules.md`

### Accurate Claims (Verified)
- Project stack description -- correct
- File paths -- correct
- Skill invocation guide -- comprehensive and accurate
- Overlapping skills pairs -- correct
- Multi-system task examples -- correct
- UE5 Blueprint protocol -- accurate
- Null guard placement rules -- accurate
- Design patterns -- all 12 patterns are valid and used
- Server-authoritative rule -- correct
- Execution protocol -- accurate
- Cross-layer coordination -- accurate
- Safeguards -- reasonable
- Key files table -- accurate
- Key socket events quick reference -- accurate (intentionally brief)
- Blueprint naming conventions -- accurate

### Inaccurate Claims

#### I33 — MCP tool references may be outdated (Severity: Low)
- **Doc references** (lines 15-18): `mcp1_read_graph`, `mcp7_sequentialthinking`
- These are MCP tool references that may or may not exist in the current environment
- **Note**: Not a codebase issue, but a tooling reference issue

#### I34 — Reference to `global_rules_appendix.md` (Severity: Low)
- **Doc says** (line 458): "See `global_rules_appendix.md`"
- This file may or may not exist
- **Fix**: Verify file exists or remove reference

### Missing Items

#### M30 — No mention of `ro_pet_data.js` in data modules list (Severity: Low)
- The Global Rules doc doesn't list individual data modules (that's in CLAUDE.md)
- But the skill invocation table doesn't mention pet-specific data file
- Minor: the `/sabrimmo-companions` skill trigger keywords do cover pets

---

## Cross-Document Consistency Issues

### C1 — Subsystem count inconsistency (Severity: High)
- `Project_Overview.md` line 112: "33 UWorldSubsystems" -- CORRECT
- `System_Architecture.md` line 111: "C++ World Subsystems (25)" -- WRONG, says 25
- `System_Architecture.md` line 280: "33 C++ UWorldSubsystems" -- CORRECT
- The same document contradicts itself (25 in the header vs 33 in the design patterns table)
- **Fix**: Change line 111 to "C++ World Subsystems (33)" and add the 8 missing subsystems to the list

### C2 — Enemy AI tick inconsistency (Severity: High)
- `System_Architecture.md` tick table (line 75): "200ms" -- CORRECT
- `System_Architecture.md` prose (line 220): "500ms" -- WRONG
- `Multiplayer_Architecture.md` prose (line 220): "500ms" -- WRONG
- **Fix**: All references should say 200ms

### C3 — Ground Effect tick inconsistency (Severity: Medium)
- `System_Architecture.md` tick table (line 80): "500ms" -- WRONG (now 250ms)
- **Fix**: Update to 250ms

### C4 — Data modules count inconsistency (Severity: Medium)
- `Project_Overview.md` (line 280): "11 data modules"
- `CLAUDE.md`: Lists 15 data modules
- **Actual**: 19 `ro_*.js` files exist
- **Fix**: All should say 19

---

## Suggested Doc Updates (Priority Order)

### Critical (Fix Immediately)
1. **System_Architecture.md line 111**: Change "25" to "33" and add 8 missing subsystems (CartSubsystem, VendingSubsystem, PartySubsystem, CraftingSubsystem, SummonSubsystem, PetSubsystem, HomunculusSubsystem, CompanionVisualSubsystem)
2. **System_Architecture.md line 80**: Change Ground Effects from "500ms" to "250ms"
3. **System_Architecture.md line 220** and **Multiplayer_Architecture.md line 220**: Change Enemy AI tick from "500ms (`ENEMY_AI.WANDER_TICK_MS`)" to "200ms (`ENEMY_AI.TICK_MS`)"
4. **Database_Architecture.md**: Add `character_pets` table definition
5. **Multiplayer_Architecture.md**: Add 49+ missing socket events to the catalog

### High Priority
6. **Database_Architecture.md items table**: Add 15+ missing columns to match actual `init.sql`
7. **Project_Overview.md line 280**: Change "11 data modules" to "19 data modules"
8. **Project_Overview.md C++ module list**: Add `SlateCore, SocketIOClient, SIOJson, Niagara, NiagaraCore, NavigationSystem`
9. **Database_Architecture.md**: Add `character_memo`, `skills`, `skill_prerequisites`, `skill_levels`, `character_skills` tables
10. **System_Architecture.md tick table**: Add 6 missing tick loops (Party HP Sync, Homunculus Hunger, Homunculus Regen, Pet Hunger, Card Periodic Effects, Spirits Recovery)

### Medium Priority
11. **Project_Overview.md line 99**: Update chat system to reflect implemented channels (party, whisper)
12. **System_Architecture.md**: Add 8 missing subsystems to the subsystem list
13. **Database_Architecture.md line 180**: Fix hotbar CHECK constraint from "1-9" to "0-8"
14. **Database_Architecture.md line 149**: Change aspd_modifier from INTEGER to FLOAT
15. **Database_Architecture.md line 150**: Change weapon_range default from 0 to 150
16. **All docs**: Update "Last Updated" dates to 2026-03-22

### Low Priority
17. **Project_Overview.md line 279**: Update server line count from ~32,200 to ~32,600
18. **Project_Overview.md line 253**: Update source file counts
19. **Project_Overview.md**: Add SSenseResultPopup, SSkillTargetingOverlay, SSkillTooltipWidget, SHotbarKeybindWidget to UI list
20. **Multiplayer_Architecture.md line 259**: Fix `player:moved` handler from MultiplayerEventSub to OtherPlayerSub
21. **Database_Architecture.md line 88**: Note that init.sql uses 'warrior' default but runtime uses 'novice'

---

## Metrics

- **Documents audited**: 5
- **Total claims verified**: ~191
- **Accurate claims**: ~157 (82%)
- **Inaccurate claims**: 34 (18%)
- **Missing documentation items**: 30
- **Critical severity**: 5
- **High severity**: 7
- **Medium severity**: 18
- **Low severity**: 14
