# Development & Reference Documentation Sync Audit

**Date**: 2026-03-22
**Auditor**: Claude Opus 4.6 (automated cross-reference against codebase)
**Scope**: 20 documentation files across Integration, Development, and Reference sections
**Method**: Each doc read in full, then cross-referenced against `server/src/index.js`, C++ subsystem headers, and MEMORY.md project state

---

## Summary

| Status | Count | Docs |
|--------|-------|------|
| OBSOLETE (plan completed, doc has stale TODO items) | 5 | Strategic_Implementation_Plan_v3, Phase_7_9_12, AC_HUDManager, Persistent_Socket_v2, Master_Migration_Plan |
| OUTDATED (partially accurate but key facts wrong) | 10 | API_Reference, Event_Reference, Configuration_Reference, Glossary, ID_Reference, Test_Checklist, UI_Testing_System, Networking_Protocol, Skill_VFX_Implementation_Plan, Skill_VFX_Execution_Plan |
| CURRENT (accurate) | 3 | Authentication_Flow, Migration_Status_08, Zone_System_Setup_Guide |
| COMPLETED PLAN (marked as done, accurate) | 2 | Phase_16_Companions_Plan, Troubleshooting |

---

## Per-Document Findings

### 1. `docsNew/04_Integration/Authentication_Flow.md`

**Status: CURRENT**
**Last Updated in doc: 2026-03-09**

- JWT `expiresIn: '24h'` confirmed at index.js lines 31553, 31615
- `authenticateToken` middleware confirmed on the correct endpoints
- Registration auto-login confirmed
- bcrypt 10 rounds confirmed
- Rate limiting 100/15min on `/api/*` confirmed

**Minor issues:**
- Line 87 lists `PUT /api/characters/:id/position` but omits `DELETE /api/characters/:id` (exists at line 31831) and `GET /api/servers` (exists at line 31670)
- The doc says "Applied to:" and lists 5 endpoints using `authenticateToken`, but misses `DELETE /api/characters/:id` which also uses it
- The doc references `WBP_LoginScreen` (line 12, 92) which is a UMG widget name. The login is now handled by `ULoginFlowSubsystem` + Slate widgets (`SLoginWidget`). The reference is architecturally stale though the auth flow logic is still accurate.

**Suggested updates:**
1. Add `DELETE /api/characters/:id` and `GET /api/servers` to the endpoint list
2. Update client-side references from `WBP_LoginScreen` to `ULoginFlowSubsystem` / `SLoginWidget`

---

### 2. `docsNew/04_Integration/Networking_Protocol.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-03-14**

**Accurate:**
- JSON format examples are correct
- Position update/broadcast format correct
- Socket.io emit patterns correct
- Client-side event routing section is accurate (0 bridges)
- Card compound event format correct
- Homunculus event table correct

**Inaccuracies found:**
1. **Currency name**: Doc uses `zuzucoin` in inventory payload (line 114). Code also uses `zuzucoin` (confirmed). However the Glossary and other docs sometimes reference "Zuzucoin" vs "zeny" inconsistently. The server has both `zuzucoin` and `zeny` properties (index.js line 3206-3207: `attacker.zuzucoin = attacker.zeny`). The doc should clarify this dual naming.
2. **ID ranges**: Line 224 says Item IDs are `1001-4999` with ranges `1001-1005 (consumable), 2001-2008 (etc), 3001-3006 (weapon), 4001-4003 (armor)`. This is massively outdated -- the actual item database has 6,169 canonical rAthena items. The original seed ranges are no longer representative.
3. **Missing events**: The doc covers homunculus events but is missing many socket events that now exist: `party:*` (9 events), `pet:*` (6 events), `vending:*` (4 events), `cart:*` (5 events), `identify:select`, `pharmacy:craft`, `crafting:craft_converter`, `summon:detonate`, `refine:request`, `forge:request`, `equipment:repair`, `warp_portal:confirm`, `inventory:merge`, `shop:buy_batch`, `shop:sell_batch`, `job:change`, `mount:toggle`, `player:sit`, `player:stand`, `buff:request`
4. **Chat channels**: Line 309 says "Currently only `GLOBAL` implemented. Future: ZONE, PARTY, GUILD, TELL." -- PARTY chat and WHISPER are now fully implemented

**Suggested updates:**
1. Update item ID ranges to reflect the 6,169 canonical item database
2. Add sections for party, pet, vending, cart, crafting, refine, forge events
3. Update chat channels from "only GLOBAL" to include PARTY and WHISPER
4. Add weight:status event format (already in Event_Reference but missing here)

---

### 3. `docsNew/05_Development/Strategic_Implementation_Plan_v3.md`

**Status: OBSOLETE (progress tracker not updated)**
**Last Updated in doc: 2026-03-14**

**Critical inaccuracies in Progress Tracker (top of doc):**

| Phase | Doc says | Actual state |
|-------|----------|-------------|
| Phase 6: Party System | "NOT STARTED" | **COMPLETE** (2026-03-17, polished 2026-03-22) -- PartySubsystem + SPartyWidget, 9 socket handlers, EXP sharing, party chat |
| Phase 7: Chat Expansion | "PARTIALLY COMPLETE" | **COMPLETE** -- Whisper system fully implemented (2026-03-18), /w /r /ex /exall /in /inall /am all working |
| Phase 8: Second Classes | "NOT STARTED" | **COMPLETE** -- All 13 second classes fully playable (Knight, Priest, Wizard, Hunter, Assassin, Blacksmith, Crusader, Sage, Bard, Dancer, Monk, Rogue, Alchemist) with 293 skill definitions, 180+ handlers |
| Phase 9: Monster Skills | "NOT STARTED" | **COMPLETE** (2026-03-17) -- ro_monster_skills.js, 27 monsters with skills, MVP system, slave summoning |
| Phase 12: Items Deep Dive | "PARTIALLY COMPLETE" | **COMPLETE** -- Refine ATK system (2026-03-17), forging system (Phase 5C), cart/vending/identify (2026-03-19) all done |

**Additional inaccuracies in body text:**
- Line 20: "Phase 6: Party System | NOT STARTED" -- completely wrong
- Line 21: "Phase 7: Chat Expansion | PARTIALLY COMPLETE" -- whisper is now done
- Line 23: "Phase 8: Second Classes | NOT STARTED" -- all 13 are done
- Line 24: "Phase 9: Monster Skills | NOT STARTED" -- done
- Line 33: "Executive Summary" mentions "33 C++ UWorldSubsystems" -- actual count is **34** (33 in UI/ + 1 in VFX/)
- Line 34: "180+ active handlers" -- likely higher now with all class audits completed
- Line 365: Phase 6 description says "NEXT PRIORITY" -- completed 5 days ago
- Line 406: Phase 7 "Still needed: Whisper, party chat" -- both are implemented

**Suggested action:** Major update needed. All phases through 12 are complete. The progress tracker and phase descriptions need rewriting to reflect actual completion status.

---

### 4. `docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md`

**Status: OBSOLETE (correctly marked as COMPLETE at top)**
**Last Updated in doc: 2026-03-14**

The doc correctly notes at the very top: "COMPLETE (2026-03-14) -- AC_HUDManager deleted. All 42 functions replaced by 25 C++ Slate subsystems."

**Issue:** The subsystem count "25" is now outdated. The actual count is **34** UWorldSubsystems (33 in UI/ + 1 in VFX/). Subsystems added after 2026-03-14 include: PartySubsystem, PetSubsystem, HomunculusSubsystem, CompanionVisualSubsystem, CartSubsystem, VendingSubsystem, CraftingSubsystem, SummonSubsystem, ItemInspectSubsystem.

**No action needed** -- this is a historical document and correctly marked as complete. The subsystem count is just a minor detail.

---

### 5. `docsNew/05_Development/Blueprint_To_CPP_Migration/00_Master_Migration_Plan.md`

**Status: OBSOLETE (correctly marked as ALL 6 PHASES COMPLETE)**
**Last Updated in doc: 2026-03-14**

- Correctly marked as complete
- Historical content preserved and accurate for when it was written
- References "19 C++ subsystems" as existing pre-migration (line 71) -- now 34 total
- Section 4.5 mentions "19 C++ Slate subsystems" -- now 34

**No action needed** -- historical document.

---

### 6. `docsNew/05_Development/Blueprint_To_CPP_Migration/08_Migration_Status_And_Next_Phase.md`

**Status: CURRENT**
**Last Updated in doc: 2026-03-14**

- Correctly shows all 6 phases + BP bridge Phases A-F as COMPLETE
- "25 C++ subsystems" in the table (line 127-153) is now outdated (34 actual), but this doc was snapshot at 2026-03-14 and additional subsystems were added later
- Event flow diagram is accurate
- BP bridge removal table is accurate (all 14 removed)
- Hard-won lessons section is valuable and accurate

**Minor issue:** The subsystem count table lists 25 subsystems. Missing the 9 added after 2026-03-14: PartySubsystem, PetSubsystem, HomunculusSubsystem, CompanionVisualSubsystem, CartSubsystem, VendingSubsystem, CraftingSubsystem, SummonSubsystem, ItemInspectSubsystem.

**Suggested update:** Add the 9 new subsystems to the "25 C++ Subsystems" table for completeness.

---

### 7. `docsNew/05_Development/Test_Checklist.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-19**

**Stale references:**
1. Line 25: "Player name tag follows character | WBP_PlayerNameTag visible above head" -- WBP_PlayerNameTag deleted in Phase 6. Name tags are now handled by `UNameTagSubsystem` + `SNameTagOverlay`
2. Lines 39-40: "WBP_GameHUD shows red HP bar" -- This is now `UBasicInfoSubsystem` + Slate widget, not WBP_GameHUD
3. Lines 44-46: "Press I or inventory key -> Inventory opens | WBP_InventoryWindow" -- Now `UInventorySubsystem` + Slate widget, key is F6
4. Line 46: "Press S or stat key -> Stat window opens | WBP_StatAllocation" -- Now `UCombatStatsSubsystem` + Slate widget, key is F8
5. Line 119: "WBP_GameHUD target frame" -- Now CombatActionSubsystem
6. Line 125: "WBP_DeathOverlay" -- Now SDeathOverlayWidget in CombatActionSubsystem
7. Line 133: "WBP_LootPopup" -- Now SLootNotificationOverlay in InventorySubsystem
8. Line 169: "BP_OtherPlayerCharacter spawned" -- Correct, but spawned by UOtherPlayerSubsystem (not BP_OtherPlayerManager)
9. Lines 187: "WBP_HoverOverIndicator" -- Now UTargetingSubsystem handles hover

**Missing test sections for systems added since 2026-02-19:**
- Skill tree UI (F9 toggle, skill learning, hotbar assignment)
- Buff bar (buff icons, countdown timers)
- Cast bar (cast time progress)
- Equipment panel (F7 toggle, equip slots)
- Hotbar system (F5 cycle, 4 rows, skill casting from hotbar)
- Zone transitions (warp portals, Butterfly Wing)
- Party system (create, invite, EXP share, HP sync)
- Whisper system (/w, /r, /ex, block)
- Cart/Vending system
- Pet/Homunculus system
- Combat log (element effectiveness, debuff messages)
- Name tags (hover-only for monsters, always for players)
- Damage number types (critical, miss, heal, block, element immune)
- Weight system (overweight thresholds)

**Known Issues table:** Line 208 -- the Mosswort HP=5 issue is likely no longer relevant since monster templates are now from canonical rAthena data (509 templates).

**Suggested action:** Comprehensive rewrite needed. All Widget Blueprint references need to be replaced with C++ Slate subsystem names. Add 14+ new test sections for post-February features.

---

### 8. `docsNew/05_Development/Troubleshooting.md`

**Status: CURRENT (recently updated)**
**Last Updated in doc: 2026-03-19**

- Recent bug fixes section is valuable and accurate
- Socket.io events section correctly references GameInstance socket (line 55)
- References UOtherPlayerSubsystem correctly (line 67)
- Position saving section (line 101) says "auto-save every 5s in Blueprint" -- this is partially stale. Position saving is now also via `UPositionBroadcastSubsystem` at 30Hz (actual position broadcast) and the 5s timer in Level Blueprint for DB persistence.

**Minor issues:**
- Line 101: "auto-save every 5s in Blueprint" -- could clarify that position broadcast is 30Hz via C++ PositionBroadcastSubsystem, with 5s DB persist via Level Blueprint timer
- Line 174: "Socket.io function param not named `Data`" -- this is a BP_SocketManager-specific pattern that is no longer relevant (BP_SocketManager is dead code)

**Suggested updates:** Minor -- remove/mark BP_SocketManager-specific tips as obsolete.

---

### 9. `docsNew/05_Development/UI_Testing_System.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-23**

**Stale references:**
1. Line 54: `Test_HUDManagerFound()` -- described as "Legacy test (AC_HUDManager replaced by C++ Slate subsystems)" in the doc itself. The doc acknowledges this is stale.
2. Line 108: "HUD Manager | Legacy AC_HUDManager detection | STALE | Needs rewrite" -- correctly flagged but never fixed
3. Line 276: Troubleshooting says to check for subsystem availability but the test code was never updated
4. All references to `ASabriMMOUITests` and `BP_AutomationTestLibrary` -- unclear if these still exist/compile with current codebase
5. The coverage table shows 5 tests total -- extremely low coverage for 34 subsystems

**Issue:** The testing system appears abandoned since 2026-02-23. No tests for any system added after that date (skills, buffs, equipment, hotbar, chat, party, etc.)

**Suggested action:** Either update the testing system to cover current subsystems or mark the document as ARCHIVED with a note about the testing gap.

---

### 10. `docsNew/06_Reference/API_Reference.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-17**

**REST Endpoint issues:**
- Doc lists 9 endpoints + 2 unlisted = "Total: 11 REST endpoints"
- Actual endpoints found in code: 11 (GET /health, POST /api/auth/register, POST /api/auth/login, GET /api/auth/verify, GET /api/servers, GET /api/characters, POST /api/characters, GET /api/characters/:id, DELETE /api/characters/:id, PUT /api/characters/:id/position, GET /api/test)
- Doc is missing `DELETE /api/characters/:id` and `GET /api/servers` from the table

**Socket.io Event issues (major):**
- Doc lists events as of 2026-02-17 -- massively incomplete
- Missing event categories:
  - **Party events** (9): party:create, party:invite, party:invite_respond, party:leave, party:kick, party:change_leader, party:change_exp_share, party:chat, party:load
  - **Pet events** (6): pet:tame, pet:incubate, pet:return_to_egg, pet:feed, pet:rename, pet:list
  - **Homunculus events** (6): homunculus:feed, homunculus:command, homunculus:skill_up, homunculus:use_skill, homunculus:evolve (+ server-to-client events)
  - **Cart events** (5): cart:load, cart:rent, cart:remove, cart:move_to_cart, cart:move_to_inventory
  - **Vending events** (4): vending:start, vending:close, vending:browse, vending:buy
  - **Crafting events** (3): pharmacy:craft, crafting:craft_converter, summon:detonate
  - **Forging/Refining events** (2): refine:request, forge:request
  - **Other missing**: identify:select, warp_portal:confirm, inventory:merge, shop:buy_batch, shop:sell_batch, equipment:repair, card:compound, job:change, mount:toggle, player:sit, player:stand, buff:request, hotbar:clear, hotbar:save_skill, zone:warp, zone:ready, kafra:open, kafra:save, kafra:teleport, skill:learn, skill:reset, skill:use, combat:respawn (all missing)
- Doc shows `enemy:attack` event is missing from the Enemy Events table (but exists in code at line 30612)
- Total socket event handlers in code: 81 (from `socket.on` grep). Doc claims "79" in the header (line 4 of Event_Reference) but only lists ~30

**Player stats payload:**
- Doc shows ASPD cap as 190 (line 9 in Glossary). Actual code has `ASPD_CAP: 195` (index.js line 344).

**Suggested action:** Major rewrite needed. Add 40+ missing socket events to the catalog.

---

### 11. `docsNew/06_Reference/Configuration_Reference.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-17**

**Server constant inaccuracies:**
1. `DEFAULT_ASPD: 180` in doc -- actual code: `DEFAULT_ASPD: 175` (index.js line 343)
2. `ASPD_CAP: 190` in doc -- actual code: `ASPD_CAP: 195` (index.js line 344)
3. `PORT` default: Doc says "3000 (typically set to 3001)" -- code confirms `process.env.PORT || 3000` (line 129)
4. `.env` variable `REDIS_URL` mentioned in CLAUDE.md but the actual code uses `REDIS_HOST` (index.js line 31437). Doc correctly lists `REDIS_HOST`.

**Database defaults:**
- `class: 'warrior'` in doc (line 119) -- actual code uses `'novice'` as the initial job class (index.js line 2078). New characters start as Novice, not Warrior.

**C++ Character Defaults:**
- The `ACombatCharacter` section (lines 102-112) -- unclear if this class still exists or was merged/removed during migration. The primary character class is `ASabriMMOCharacter`.

**Missing configurations:**
- No mention of `COMBAT.COMBAT_TICK_MS` (50ms confirmed correct)
- Missing `INVENTORY.MAX_WEIGHT` -- code uses `getPlayerMaxWeight()` function, not a constant
- Missing `ENEMY_AI` section doesn't reflect actual RO monster AI codes (509 templates with per-monster AI)

**Suggested updates:**
1. Fix DEFAULT_ASPD from 180 to 175
2. Fix ASPD_CAP from 190 to 195
3. Fix default class from 'warrior' to 'novice'
4. Verify ACombatCharacter still exists

---

### 12. `docsNew/06_Reference/Event_Reference.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-03-13**

(See API_Reference findings above -- the Event_Reference shares the same socket event catalog issues.)

**Additional specific issues:**
1. Line 4: "Total: 79 socket event handlers" -- actual grep count is 81+
2. Line 5 note says "Events added in later phases... are documented in their respective system docs but not yet added to this reference" -- this is honest but means the reference is incomplete
3. Chat events (line 308-309): "Currently only GLOBAL implemented. Future: ZONE, PARTY, GUILD, TELL" -- PARTY and WHISPER are now implemented
4. `combat:damage` payload (line 129-152): Shows dual wield fields which is correct and up-to-date
5. `combat:auto_attack_stopped` reasons (line 126): Missing "Overweight" reason (added with weight system)
6. Weight events section (lines 700-728): Present and accurate -- good
7. `skill:data` response (line 614-638): Missing `sharedTreePos`, `iconClassId` fields added for Crusader shared skills
8. `enemy:attack` event: Exists in code but NOT listed in the Enemy Events section (line 49-56). This is a significant omission.
9. `hotbar:save_skill`, `hotbar:clear`, `hotbar:request` events are documented -- good
10. `inventory:move` and `inventory:merge` not documented (merge was added 2026-03-18)

**Suggested action:** Add enemy:attack event. Add 40+ missing events or add section references to system-specific docs. Update chat channel status.

---

### 13. `docsNew/06_Reference/Glossary.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-17**

**Inaccuracies:**
1. ASPD definition (line 9): "Scale 0-190 where higher is faster. ASPD 180 = 1 attack/sec, 190 = 2/sec" -- ASPD_CAP is actually 195, not 190. The attack interval formula `(200 - ASPD) * 50` is in the Stat Formulas section and is correct (but the scale max is wrong).
2. ASPD formula (line 86): `min(190, floor(170 + AGI*0.4 + DEX*0.1))` -- should be `min(195, ...)` based on ASPD_CAP. Also the base ASPD formula is class-dependent and weapon-type-dependent in the actual code, not a flat `170 + AGI*0.4 + DEX*0.1`.
3. Stat Points (line 16): "New characters start with 48" -- confirmed correct (index.js line 5378)
4. Item ID ranges (lines 39-43): Shows old seed data ranges (22 total items). Actual database has 6,169 items. These ranges are vestigial.
5. maxHP formula (line 87): `100 + VIT*8 + Level*10` -- simplified. Actual RO Classic formula is class-dependent and uses HP growth tables from `ro_exp_tables.js`.
6. maxSP formula (line 88): `50 + INT*5 + Level*5` -- same issue, simplified vs actual class-dependent formula
7. Physical Damage formula (line 90-92): Extremely simplified. Actual damage pipeline has 10+ steps (ro_damage_formulas.js) including element, size, race, card mods, refine ATK, mastery, etc.
8. Socket.io Event Naming Convention (lines 67-73): Missing domains: `party:`, `pet:`, `homunculus:`, `cart:`, `vending:`, `pharmacy:`, `crafting:`, `summon:`, `refine:`, `forge:`, `equipment:`, `warp_portal:`, `identify:`, `zone:`, `kafra:`, `mount:`, `buff:`, `status:`, `job:`, `weight:`, `skill:`, `debug:`
9. File Naming Conventions: WBP_ described as "Widget Blueprint (legacy, all replaced by C++ Slate widgets)" -- correctly marked as legacy. AC_ described as "Actor Component (legacy, all replaced by C++ UWorldSubsystems)" -- correctly marked as legacy.

**Suggested action:** Update ASPD scale to 195, update item ID ranges, add note about simplified formulas, add missing socket event domains.

---

### 14. `docsNew/06_Reference/ID_Reference.md`

**Status: OUTDATED**
**Last Updated in doc: 2026-02-19**

**Major inaccuracy:**
- The entire item ID section documents 22 items (the original seed data). The actual database has **6,169 canonical rAthena items** imported during Phase 5 (2026-03-10). The documented consumables (1001-1005), loot (2001-2008), weapons (3001-3006), and armor (4001-4003) are vestigial seed data that may no longer even exist in their original form.

**Enemy template section:**
- Lists 6 enemy templates (blobby, crawlid, hoplet, mosswort, shroomkin, buzzer). Actual codebase has **509 RO monster templates** in `ro_monster_templates.js` and 46 active spawn points.
- Spawn configuration shows 12 total enemies -- massively outdated
- Drop tables show the original seed drops -- no longer representative

**Skill IDs not documented:**
- The doc has no skill ID section despite skills being a major system (293 definitions across 7 class groups)

**Suggested action:** This doc needs a fundamental rethink. The seed data it documents has been superseded by canonical rAthena imports. Consider:
1. Adding a Skill ID section (Novice 1-3, Swordsman 100-109, Mage 200-213, etc.)
2. Replacing the 22-item section with a summary of the 6,169-item database structure
3. Replacing the 6-template enemy section with a summary of 509 templates
4. Keeping the ID range conventions (Enemy starts at 2000001, etc.)

---

### 15. `docsNew/05_Development/Skill_VFX_Implementation_Plan.md`

**Status: OUTDATED (partially -- code architecture exists, Niagara assets pending)**
**Last Updated in doc: 2026-03-05**

- Status correctly says "FUTURE PLAN -- VFX configs defined in code (97+), asset mapping complete, Niagara build pending"
- The C++ `SkillVFXSubsystem` (97+ configs) and `CastingCircleActor` are implemented
- Niagara module dependencies and actual particle assets are NOT yet built
- "Skills Needing VFX (15 implemented server-side)" at line 33 -- massively outdated. There are now 180+ active skill handlers, not 15.
- The "Current State" section only lists first-class skills. All 13 second classes are now implemented.

**Suggested update:** Update the "Current State" section to reflect the actual 180+ skills. The Niagara pipeline plan itself is still valid for future work.

---

### 16. `docsNew/05_Development/Skill_VFX_Execution_Plan.md`

**Status: OUTDATED (partially executed)**
**Last Updated in doc: 2026-03-05**

- Phase 3 (C++ Core SkillVFXSubsystem) and Phase 4 (CastingCircleActor) were completed
- Phase 0 (prerequisites) was partially done (VFX directory exists)
- Phases 1-2 (MCP automation/fallback) -- status unknown
- Phases 5-11 (materials, Niagara, textures, wiring, testing) -- not yet executed
- The doc is still valid as a future execution plan but should note which phases are done

**Suggested update:** Mark Phases 0/3/4 as COMPLETE, note remaining phases.

---

### 17. `docsNew/05_Development/Phase_16_Companions_Plan.md`

**Status: CURRENT (correctly marked COMPLETED)**
**Last Updated in doc: 2026-03-18**

- Tasks 1-4 all marked as COMPLETE -- accurate per MEMORY.md entries
- Task 5 (Mercenary) correctly marked as DEFERRED
- Pet system details (48 pets, taming, feeding, intimacy) match implementation
- Homunculus details (4 types, 12 skills, combat tick, evolution) match
- Companion visuals (placeholder actors, 10Hz follow) match

**No action needed.**

---

### 18. `docsNew/05_Development/Phase_7_9_12_Completion_Plan.md`

**Status: OBSOLETE (correctly marked COMPLETED)**
**Last Updated in doc: 2026-03-18**

- All tasks in the progress tracker are marked COMPLETE -- accurate
- Death penalty, whisper system, MVP system, slave summoning, monster skill DB all confirmed implemented
- "Pre-Audit Findings" section correctly identifies Boss/Normal card category and class weapon restrictions as already implemented

**No action needed** -- this is a completed plan document.

---

### 19. `docsNew/05_Development/Persistent_Socket_Connection_Plan_v2.md`

**Status: OBSOLETE (correctly marked COMPLETE)**
**Last Updated in doc: 2026-03-10 (with 2026-03-14 update note)**

- All sub-phases marked COMPLETE -- accurate
- The update note correctly states "all bridges removed (BP bridge migration Phases A-F complete, 14->0 bridges)"
- Technical discoveries section is valuable and accurate
- Historical "Current vs Target" architecture diagrams are correctly labeled

**Issue:** The body text (Phase 4a-4d sections starting at line 209) describes the original plan using `USocketIOClientComponent` on GameInstance. The actual implementation used `TSharedPtr<FSocketIONative>` directly. The progress tracker correctly reflects the actual implementation, but the detailed plan sections describe the original approach.

**No action needed** -- historical document with correct status markers.

---

### 20. `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md`

**Status: CURRENT**

- Correctly notes BP_SocketManager, BP_OtherPlayerManager, and BP_EnemyManager are no longer needed
- Level Blueprint spawn flow is accurate
- GameMode override to None requirement is correctly documented
- Warp portal placement instructions are actionable
- NavMesh setup instructions are valid

**Minor issue:** Line 18 says "zone registry (4 zones)" -- the actual zone count may have grown. But the guide itself is still accurate for setup procedures.

**No action needed.**

---

## Cross-Cutting Issues

### 1. Subsystem Count Inconsistency

Multiple docs reference different subsystem counts:
- CLAUDE.md: "30+ UWorldSubsystem files in `UI/`"
- Strategic Plan: "33 C++ UWorldSubsystems"
- Migration Status: "25 C++ subsystems"
- AC_HUDManager Plan: "25 C++ Slate subsystems"

**Actual count (from glob):** 33 in UI/ + 1 in VFX/ = **34 total UWorldSubsystems**

### 2. ASPD Cap Inconsistency

- Glossary says 190
- Configuration Reference says 190
- Actual code: `ASPD_CAP: 195`

### 3. Default ASPD Inconsistency

- Configuration Reference says 180
- Actual code: `DEFAULT_ASPD: 175`

### 4. Default Character Class

- Configuration Reference says `'warrior'`
- Actual code: `'novice'` (new characters start as Novice)

### 5. Currency Naming

- Some docs use "Zuzucoin", some use "zuzucoin", some use "zeny"
- Server code has both: `player.zuzucoin` and `player.zeny` kept in sync
- Should standardize terminology across all docs

### 6. Item Database Scale

- Multiple docs reference the original 22 seed items
- Actual database: 6,169 canonical rAthena items
- Docs written before Phase 5 (2026-03-10) are all wrong on item counts

### 7. Monster Template Scale

- Multiple docs reference 6 custom templates or "12 enemies"
- Actual: 509 RO monster templates, 46 active spawn points
- Docs written before the RO data import are all wrong on monster counts

---

## Priority Action Items

### HIGH PRIORITY (impacts active development)

1. **Strategic_Implementation_Plan_v3.md** -- Update progress tracker. Phases 6-9 and 12 are all COMPLETE but marked as NOT STARTED. This is the most misleading doc.
2. **Configuration_Reference.md** -- Fix ASPD_CAP (190->195), DEFAULT_ASPD (180->175), default class ('warrior'->'novice'). These affect anyone reading config reference.
3. **Glossary.md** -- Fix ASPD scale (190->195). Fix stat formulas or add disclaimer about simplified versions.

### MEDIUM PRIORITY (reference accuracy)

4. **API_Reference.md** -- Add DELETE /api/characters/:id, GET /api/servers. Add 40+ missing socket events (or link to system docs).
5. **Event_Reference.md** -- Add enemy:attack event. Add missing socket event categories. Update chat channels.
6. **Test_Checklist.md** -- Replace all WBP_ widget references with Slate subsystem names. Add 14+ missing test sections.
7. **Networking_Protocol.md** -- Update item ID ranges, add missing event categories, update chat channels.
8. **ID_Reference.md** -- Fundamental rethink needed for 6,169-item and 509-monster scale.

### LOW PRIORITY (historical docs)

9. **UI_Testing_System.md** -- Either update or archive as dormant
10. **Skill_VFX_Implementation_Plan.md** -- Update "Current State" skill count
11. **Skill_VFX_Execution_Plan.md** -- Mark completed phases
12. **Authentication_Flow.md** -- Minor: add missing endpoints, update widget name

---

**Audit completed: 2026-03-22**
