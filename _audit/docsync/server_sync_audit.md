# Server Documentation Sync Audit

**Auditor**: Claude (automated)
**Date**: 2026-03-22
**Scope**: 11 server-side documentation files vs actual code in `server/src/index.js` (32,566 lines) + 19 data modules

---

## Summary

| Document | Accuracy | Issues Found | Severity |
|----------|----------|-------------|----------|
| NodeJS_Server.md | ~55% | 28 | 8 High, 14 Med, 6 Low |
| API_Documentation.md | ~70% | 8 | 3 High, 4 Med, 1 Low |
| Combat_System.md | ~75% | 9 | 2 High, 5 Med, 2 Low |
| Enemy_System.md | ~80% | 7 | 1 High, 4 Med, 2 Low |
| Skill_System.md | ~50% | 18 | 6 High, 8 Med, 4 Low |
| Inventory_System.md | ~85% | 5 | 1 High, 3 Med, 1 Low |
| EXP_Leveling_System.md | ~90% | 3 | 0 High, 2 Med, 1 Low |
| Status_Effect_Buff_System.md | ~60% | 12 | 4 High, 6 Med, 2 Low |
| Pet_System.md | ~85% | 3 | 1 High, 1 Med, 1 Low |
| Card_System.md | ~70% | 7 | 3 High, 3 Med, 1 Low |
| Monster_Skill_System.md | ~55% | 9 | 4 High, 3 Med, 2 Low |

**Total Issues**: 109
**Most Outdated**: Skill_System.md (last updated 2026-03-10, missing all second-class handler info), NodeJS_Server.md (massive undercounting of events, modules, features)

---

## 1. NodeJS_Server.md

**Last Updated (doc)**: 2026-03-09
**Accuracy**: ~55%

### HIGH Severity

| # | Issue | Doc Says | Code Shows | Location |
|---|-------|----------|------------|----------|
| 1 | Data module count | "11 data modules (~6,000 lines)" | 19 data module files (5,520 lines for 9 measured + ~8,000 for remaining 10) | Line 7 header, line 74 table |
| 2 | Socket event count (C->S) | "34 events" listed | **79 socket event handlers** total. Doc is missing 45+ events: all party events (9), all cart events (5), all vending events (4), all pet events (6), all homunculus events (5), forge/refine (2), pharmacy/crafting (2), summon, warp_portal:confirm, inventory:merge, equipment:repair, player:sit/stand, mount:toggle, buff:request, identify:select, party:load | Line 206 |
| 3 | Server events (S->C) | "43+ events" | Many more: party:*, pet:*, homunculus:*, vending:*, cart:*, arrow_crafting:*, combat:blocked, identify:*, weight:status, etc. | Line 244 |
| 4 | NPC Shops | "4 shops" listed | **5 shops** in code (added Shop 5: Arrow Dealer with 35 items) | Line 409 |
| 5 | Database tables | "9 tables" listed | At least 15+ tables: also `character_cart`, `character_pets`, `character_homunculus`, `parties`, `party_members`, `vending_shops`, `vending_items`, `character_memo` | Line 443 |
| 6 | Tick loops | "8 tick loops" | **12+ tick loops**: also Party HP Sync (1s), Spirits Recovery (1s), Card Periodic Effects (5s), Pet Hunger Tick, Homunculus Tick | Line 310 |
| 7 | Data module table missing entries | Lists 8 modules | Missing: `ro_buff_system`, `ro_status_effects`, `ro_ground_effects`, `ro_monster_skills`, `ro_item_effects`, `ro_card_prefix_suffix`, `ro_homunculus_data`, `ro_arrow_crafting`, `ro_card_effects`, `ro_item_groups`, `ro_pet_data` (11 unlisted modules) | Line 76 |
| 8 | Ground Effects tick | "500ms" | **250ms** (changed from 500ms for Storm Gust timing accuracy) | Line 320, code line 27680 |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 9 | connectedPlayers shape | Missing many fields | Missing: `cartItems`, `cartMaxWeight`, `cartWeight`, `equippedAmmo`, `armorElement`, `cardBonuses`, `cardMods`, `cardDefMods`, `isSitting`, `isVending`, `partyId`, `petBonuses`, `maximizePowerActive`, `performanceState`, `spiritSphereCount`, `plagiarizedSkillId`, `comboState`, `currentWeight`, many more |
| 10 | enemies shape | Missing fields | Missing: `_slaves`, `_masterId`, `_isSlave`, `_casting`, `softDef`, `softMDef`, `buffs` details |
| 11 | Active skills | "17 with execution handlers" | 171+ skill handlers across all 14 classes (13 second classes fully implemented) |
| 12 | Passive skills | "7 listed" | 31+ passives (all first + second class passives) |
| 13 | Buff types | "6 listed" (provoke, endure, frozen, stone_curse, sight, first_aid) | 95 buff types in `ro_buff_system.js` |
| 14 | Stat formulas section | Lists basic formulas only | Missing: Refine ATK, dual wield formula, mounted spear formula, card MaxHP/MaxSP rate, pet bonuses, party EXP share |
| 15 | File structure | Lists limited files | Missing many files: `ro_card_effects.js`, `ro_item_groups.js`, `ro_pet_data.js`, check_ranges.js |
| 16 | Tool Dealer shop items | "11 items" | **13 items** (added Magnifier 611) |
| 17 | Skill-Based Regen formula | "SP Recovery (204): Lv*3 + Lv*MaxSP/500" | Code also adds: `+ floor(MaxSP * 0.002 * Lv)` MaxSP% component AND includes Priest skill 1016 Meditation stacking |
| 18 | HP Regen formula | "max(1, floor(MaxHP/200)) + floor(VIT/5)" | Code also applies: equipHpRegenRate, cardHpRecovRate, sitting 2x, Magnificat 2x, weight blocking, hidden/play dead blocking |
| 19 | Server line count | "~32,200 lines" | 32,566 lines (minor drift) |
| 20 | `activeGroundEffects` description | "Fire Wall / Safety Wall effects" | Now also includes: Warp Portal, Pneuma, songs, dances, traps, Storm Gust, LoV, Meteor Storm, Magnus Exorcismus, Sanctuary, Ice Wall, Quagmire, Volcano, Deluge, Violent Gale, Land Protector, Demonstration, Venom Dust |
| 21 | IDLE_AFTER_CHASE_MS | 3000 | 2000 in code (line 28566) |
| 22 | ro_skill_data exports | Missing `CLASS_PROGRESSION` | Listed in code but not in doc table |

### LOW Severity

| # | Issue |
|---|-------|
| 23 | Pricing description: "Buy = item.price * 2" -- code uses `item.buy_price` with legacy fallback |
| 24 | Last Updated date is 2026-03-09, 13 days behind |
| 25 | `getAttackIntervalMs` now takes `aspdPotionReduction` parameter (ASPD potions) |
| 26 | Missing `REDIS_URL` env var from table (only `LOG_LEVEL` listed, Redis mentioned in CLAUDE.md) |
| 27 | Constants table lists `SPEED_TONICS` as "4 consumable tonic definitions" -- still accurate but the doc should note these are superseded by `sc_start` handler for canonical ASPD potions |
| 28 | `ro_item_mapping.js` still listed as import but code says "replaces ro_item_mapping.js" (runtime itemNameToId Map used instead) |

---

## 2. API_Documentation.md

**Last Updated (doc)**: 2026-02-17
**Accuracy**: ~70%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Character name limit | "2-50 chars" | Code: `2-24 characters` (line 31734) |
| 2 | Character class creation | "characterClass: warrior|mage|archer|healer|priest", "defaults to warrior" | Code: All characters forced to `'novice'` (line 31767). characterClass input is ignored |
| 3 | DELETE character | Says "requires password via bcrypt" | Need to verify -- but also missing `hairStyle`, `hairColor`, `gender` fields in POST |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 4 | POST /api/characters request body | Missing `hairStyle`, `hairColor`, `gender` | Code accepts these fields (line 31730) |
| 5 | POST /api/characters response | Shows class="mage", spawn at (0,0,0) with 100 HP/MP | Class is always "novice"; spawn is correct. Response includes `job_level`, `job_class`, `hair_style`, `hair_color`, `gender`, `stat_points` |
| 6 | Missing GET /api/servers response fields | Shows only server name + population | Need to check if more fields exist |
| 7 | Error response for name conflict | "You already have a character with this name" | Code: "Character name is already taken" (global uniqueness, not per-account) |

### LOW Severity

| # | Issue |
|---|-------|
| 8 | Last Updated: 2026-02-17 (over a month old) |

---

## 3. Combat_System.md

**Last Updated (doc)**: 2026-03-12
**Accuracy**: ~75%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | "Full RO-Style Formula (Available but not used in PvE tick)" | States simplified `BASE_DAMAGE` formula is used | Code actually uses `roPhysicalDamage()` for all combat (auto-attack tick, line ~25200). The simplified formula description is completely wrong for current state |
| 2 | SPEED_TONICS note | "not yet wired to a consumption/inventory event" | The `sc_start` handler in `inventory:use` now handles canonical ASPD potions (Concentration, Awakening, Berserk Potion). SPEED_TONICS are superseded |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 3 | softDEF formula | "VIT*0.5 + VIT^2/150" | Code in `ro_damage_formulas.js` uses `floor(VIT/2) + floor(AGI/5) + floor(Level/2)` (header formula is correct, body section is wrong) |
| 4 | Respawn position | "SPAWN_POSITION (0, 0, 300)" | Respawn actually goes to save point (save_map, save_x/y/z), which can be any zone |
| 5 | Missing systems | No mention of: Safety Wall blocking in auto-attack tick, refine ATK in damage pipeline, element modifier order, Lex Aeterna double-damage in auto-attack, Pneuma check, Auto Guard block, Cicada Skin Shed dodge, Shield Reflect |
| 6 | `getAttackIntervalMs` signature | `getAttackIntervalMs(aspd)` | Code: `getAttackIntervalMs(aspd, aspdPotionReduction)` -- ASPD potion delay reduction parameter added |
| 7 | Death processing (enemy) | Step list is simplified | Missing: death penalty (EXP loss), party EXP distribution, MVP rewards, card kill hooks, card drop bonuses |

### LOW Severity

| # | Issue |
|---|-------|
| 8 | Square root scaling description in ASPD section mentions "sqrt(agi) * 1.2" but the actual formula is WD-based, not sqrt |
| 9 | Missing description of weight blocking (>= 90% blocks attacks) |

---

## 4. Enemy_System.md

**Last Updated (doc)**: 2026-03-04
**Accuracy**: ~80%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | "Enemies spawn on server startup" (Overview) | States startup spawning | Code uses **lazy spawning** (spawn on first player zone entry). The doc body section (line 436) correctly states lazy spawning but the Overview contradicts it |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 2 | Spawn points | "46 spawn points active (zones 1-3 only)" | Zone spawns are now from `ZONE_REGISTRY` in `ro_zone_data.js` with lazy loading. Need to verify current count |
| 3 | Combat interaction hooks table | Lists only first-class skills (Bash, Bolt skills, etc.) | Missing all second-class skills: Knight (Pierce, BB), Wizard (Storm Gust, etc.), Priest (Heal vs undead), Assassin (Sonic Blow), Hunter (traps), etc. |
| 4 | AI Type table | Lists 11 codes | Code has 27 AI type codes defined in AI_TYPE_MODES |
| 5 | Missing monster skill system in enemy AI | No mention of `MONSTER_SKILL_DB` integration | Monster skill casting is fully integrated into the AI ATTACK state (checks skills before auto-attack) |

### LOW Severity

| # | Issue |
|---|-------|
| 6 | Missing: slave monster tracking (`_slaves`, `_masterId`, `_isSlave`), master-death kills slaves |
| 7 | Missing: `softDef` and `softMDef` fields on enemy data structure (added for Crusader/Wizard skills) |

---

## 5. Skill_System.md

**Last Updated (doc)**: 2026-03-10
**Accuracy**: ~50%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Total skill count | "293 skill definitions" | **291 total** (69 first-class + 222 second-class) |
| 2 | Acolyte skill count | "14 skills (IDs 400-413)" | **15 skills** (IDs 400-414, includes Holy Light 414) |
| 3 | "Second class skill handlers" listed as Future | Lists as future expansion | ALL 13 second classes have working handlers: Knight (11), Crusader (14), Wizard (14), Sage (22), Hunter (18), Bard (20+), Dancer (20+), Priest (19), Monk (16), Assassin (12), Rogue (19), Blacksmith (11), Alchemist (16) |
| 4 | Archer 305 Arrow Crafting | "deferred: Not yet implemented" | **IMPLEMENTED** (line 20308 of index.js, uses `ARROW_CRAFTING_RECIPES`) |
| 5 | Merchant 605 Vending | "deferred: Not yet implemented" | **IMPLEMENTED** (line 20294, full vending system with browse/buy) |
| 6 | Merchant 606 Item Appraisal | "deferred: Not yet implemented" | **IMPLEMENTED** (line 20249, identify:select handler) |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 7 | Merchant 607 Change Cart | "deferred" | **IMPLEMENTED** (line 20279, changes cart_type based on level) |
| 8 | "Implemented Skill Handlers (Phase 5)" heading | Implies Phase 5 is latest | All 13 second classes implemented in Phases 3-6 |
| 9 | Passive skills table | Lists 12 passives | 31+ passives in `getPassiveSkillBonuses()` including all second-class passives |
| 10 | Missing skill count for Archer | "7 skills: 4 active + 2 passive + 1 deferred" | All 7 active (no deferred), Arrow Crafting implemented |
| 11 | Missing second class skill tables | Not documented | Knight (11), Wizard (14), Sage (22), Hunter (18), Priest (19), Monk (16), Assassin (12), Rogue (19), Blacksmith (11), Alchemist (16), Bard (20+), Dancer (20+), Crusader (14) |
| 12 | ro_skill_data.js description | "69 first-class skill definitions" | 69 is correct but the summary says "68 skills" for first-class total. It's 69 (3 Novice + 10 Swordsman + 14 Mage + 7 Archer + 15 Acolyte + 10 Thief + 10 Merchant) |
| 13 | First Class Total in count summary | "68 skills" | Should be 69 (Acolyte has 15 not 14) |
| 14 | Second class count | "224 second-class skills" | 222 (grep count) |

### LOW Severity

| # | Issue |
|---|-------|
| 15 | "Quest skills" listed as Future | Quest skill NPC gates implemented (Phase J of deferred remediation) |
| 16 | Energy Coat listed both as "Future Expansion" AND strikethrough "IMPLEMENTED" |
| 17 | Hotbar `zeroBased` flag mentioned but not well documented |
| 18 | Last Updated 2026-03-10, 12 days behind |

---

## 6. Inventory_System.md

**Last Updated (doc)**: 2026-03-12
**Accuracy**: ~85%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Missing `inventory:merge` event | Not documented | `inventory:merge` handler exists (line 23628) for stack merging |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 2 | Missing `equipment:repair` event | Not documented | Handler exists (line 23284) |
| 3 | Missing cart system | No mention of `cart:load`, `cart:move_to_cart`, `cart:move_to_inventory`, `cart:rent`, `cart:remove` | Full cart system with 5 socket handlers |
| 4 | `IsConsumable()` note | "item_type === 'consumable'" check | Code also accepts `'usable'` type (e.g., Magnifier) |

### LOW Severity

| # | Issue |
|---|-------|
| 5 | Line references may be stale (doc says "~line 6322" for inventory:use, actual is line 22203) |

---

## 7. EXP_Leveling_System.md

**Last Updated (doc)**: Not explicitly dated
**Accuracy**: ~90%

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Missing party EXP distribution | No mention | `distributePartyEXP()` handles Even Share (+20%/member bonus) and tap bonus (+25%/attacker) |
| 2 | Missing death penalty | No mention | EXP loss on death implemented (configurable per base level) |

### LOW Severity

| # | Issue |
|---|-------|
| 3 | Missing MVP EXP handling (mvpExp field, MVP announcements) |

---

## 8. Status_Effect_Buff_System.md

**Last Updated (doc)**: Undated
**Accuracy**: ~60%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Status effect count | "10 status effects" | **12 status effects**: also `ankle_snare` and `petrifying` (stone curse initial phase) |
| 2 | Buff type list | Shows 24 buff types, 9 marked "Future (2nd class)" | **95 buff types** in code. ALL 9 "Future" buffs are ACTIVE (two_hand_quicken, kyrie_eleison, magnificat, gloria, lex_aeterna, aspersio, enchant_poison, cloaking, quagmire). Plus 71 more unlisted: auto_guard, defender, reflect_shield, devotion, providence, spear_quicken, steel_body, critical_explosion, fury, combo_wait, root_lock, close_confine, poison_react, auto_counter, strip_weapon/armor/helm/shield, tunneldrive, slow_poison, blade_stop, sitting, playing_dead, summon_spirit_sphere, songs/dances (whistle, assassin_cross, bragi, apple_idol, etc.), performance buffs, and many more |
| 3 | `getBuffModifiers` switch cases | Lists 17 cases | Code has cases for 95 buff types |
| 4 | `getCombinedModifiers` missing fields | Listed fields are incomplete | Missing many pass-through fields: `disableSPRegen`, `isPlayDead`, `noSizePenalty`, `preventBreak`, `preventStrip`, `castTimeReduction`, `afterCastDelayReduction`, `spCostReduction`, many more |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 5 | Damage-break paths | Lists 5 paths (auto-attack, PvP, bolts, Fire Ball, Fire Wall) | Called in 20+ paths including all second-class skill damage handlers |
| 6 | Missing buff categories | No mention of mutual exclusion groups | Code has ASPD Haste1/Haste2 groups, performance mutexes, sitting vs combat, etc. |
| 7 | Missing `BUFFS_SURVIVE_DEATH` | Not documented | Whitelist of buffs that persist through death |
| 8 | Missing `UNDISPELLABLE` set | Not documented | Set of 13+ buffs immune to Dispel |
| 9 | Stone Curse drain | "1% MaxHP/sec" | Code: "3s delay, then 1% MaxHP / 5s" (the doc table entry IS correct but conflicts with the text) |
| 10 | Missing performance system buffs | Not documented | Bard/Dancer songs and dances create buff types with stat modifiers |

### LOW Severity

| # | Issue |
|---|-------|
| 11 | Import block shown is incomplete (missing many imports) |
| 12 | debug events guard check: doc says `NODE_ENV !== 'development'`, need to verify exact guard |

---

## 9. Pet_System.md

**Last Updated (doc)**: 2026-03-18
**Accuracy**: ~85%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Pet count | "48 tameable monsters" | **54 pets** (54 `captureRate:` entries in `ro_pet_data.js`) |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 2 | Database table name | "character_pets" with specific columns | Table exists but auto-created columns may differ slightly from doc |

### LOW Severity

| # | Issue |
|---|-------|
| 3 | Hunger tick interval described as "hungryDelay ms (default 60s)" -- code uses per-pet `hungryDelay` from PET_DB |

---

## 10. Card_System.md

**Last Updated (doc)**: 2026-03-13
**Accuracy**: ~70%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | GTB (magic immunity) | "Deferred" | **IMPLEMENTED**: `cardNoMagicDamage` tracked in `rebuildCardBonuses()`, enforced in `calculateMagicalDamage()` at ro_damage_formulas.js line 939 |
| 2 | Auto-cast cards | "Deferred" (e.g., "Turtle General = Magnum Break") | **IMPLEMENTED**: `processCardAutoSpellOnAttack()` and `processCardAutoSpellWhenHit()` with `executeAutoSpellEffect()` (index.js line 3530+) |
| 3 | Drake Card (nullify size penalty) | "Deferred" | **IMPLEMENTED**: `cardNoSizeFix` in `rebuildCardBonuses()`, enforced in `calculatePhysicalDamage()` at ro_damage_formulas.js line 560 |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 4 | Status effect on-hit cards | "Deferred: Needs proc-on-hit system" | **IMPLEMENTED**: `processCardStatusProcsOnAttack()` and `processCardStatusProcsWhenHit()` (index.js lines 3247, 3283) |
| 5 | Skill-granting cards | "Deferred: Needs script engine" | Partially implemented via conditional card engine |
| 6 | "Not Yet Implemented" table | Shows 8 deferred items, 1 complete | At least 5 of the 8 "deferred" items are now implemented |

### LOW Severity

| # | Issue |
|---|-------|
| 7 | Dokebi card listed with ID 4034 but that's Goblin Card. Dokebi is 4072 |

---

## 11. Monster_Skill_System.md

**Last Updated (doc)**: 2026-03-17
**Accuracy**: ~55%

### HIGH Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 1 | Active monster count | "23 monster templates with skill data" | **27 monsters** in `MONSTER_SKILL_DB` (counted via grep) |
| 2 | Slave summoning | "Stub only; needs master-slave tracking" | **FULLY IMPLEMENTED**: `NPC_SUMMONSLAVE` with master-slave tracking (`_slaves` Set, `_masterId`, `_isSlave`), slave count limit, chase target sharing, slave death on master death (index.js line 29540+) |
| 3 | Metamorphosis | "Stub only; needs template swap mechanism" | **FULLY IMPLEMENTED**: `NPC_METAMORPHOSIS` with template property overwriting, HP percentage preservation, aggro state transfer (index.js line 29610+) |
| 4 | Ground-target monster skills | "Monsters can't place ground effects yet" | Partially implemented (monster player skills like Water Ball work via `executeMonsterPlayerSkill`) |

### MEDIUM Severity

| # | Issue | Doc Says | Code Shows |
|---|-------|----------|------------|
| 5 | Active monsters table | Lists only 10 monsters | 17 additional monsters have skills defined (Osiris, Baphomet, Drake, Angeling, Eddga, Mistress, Strouf, Orc Warrior, Archer Skeleton, Bathory, Isis, Medusa, Side Winder, Maya, Golden Thief Bug, Dark Lord, Stormy Knight) |
| 6 | Boss skills table | Lists 4 bosses as "future zones" | Some bosses are in MONSTER_SKILL_DB and will activate when zones spawn |
| 7 | Monster count in overview | "12 monsters, 40+ NPC_ skills" | 27 monsters with skill entries |

### LOW Severity

| # | Issue |
|---|-------|
| 8 | Client cast bar: "no client UI handler" -- may have been added in recent sessions |
| 9 | Monster skill VFX: "no client VFX mapping" -- SkillVFXSubsystem may handle enemy:skill_used |

---

## Major Undocumented Systems

These systems exist in the code but have NO documentation in the 11 audited server-side docs:

| System | Socket Events | Code Lines | Notes |
|--------|--------------|------------|-------|
| **Party System** | 9 handlers (party:create/invite/invite_respond/leave/kick/change_leader/change_exp_share/chat/load) | ~500 lines | Full party with EXP share, HP sync, chat |
| **Cart System** | 5 handlers (cart:load/rent/remove/move_to_cart/move_to_inventory) | ~300 lines | Cart storage for Merchant classes |
| **Vending System** | 4 handlers (vending:start/close/browse/buy) | ~400 lines | Player shops with browse/buy |
| **Identify System** | 1 handler (identify:select) | ~50 lines | Item Appraisal for Merchant |
| **Forge/Refine System** | 2 handlers (forge:request/refine:request) | ~300 lines | Weapon forging and refining |
| **Pharmacy/Crafting** | 2 handlers (pharmacy:craft/crafting:craft_converter) | ~150 lines | Alchemist potion crafting |
| **Summon System** | 1 handler (summon:detonate) | ~100 lines | Alchemist summon entities |
| **Homunculus System** | 5 handlers (feed/command/skill_up/use_skill/evolve) | ~500 lines | Full homunculus companion |
| **Warp Portal Confirm** | 1 handler (warp_portal:confirm) | ~60 lines | Destination selection popup |
| **Sitting System** | 2 handlers (player:sit/stand) | ~40 lines | Sit for regen bonus |
| **Mount System** | 1 handler (mount:toggle) | ~20 lines | Knight/Crusader riding |
| **Equipment Repair** | 1 handler (equipment:repair) | ~40 lines | Blacksmith weapon repair |
| **Buff Request** | 1 handler (buff:request) | ~30 lines | Client buff list refresh |
| **Bard/Dancer Performance** | Via skill:use | ~800 lines | Performance ground effects, ensemble system |
| **Monk Combo System** | Via skill:use + combat tick | ~200 lines | Combo state, spirit spheres |
| **Rogue Plagiarism** | Via monster skill hooks | ~100 lines | Copy monster player skills |
| **Ensemble System** | Via performance system | ~300 lines | Bard+Dancer duet skills |
| **Abracadabra System** | Via skill:use | ~200 lines | 145 regular + 6 special random skills |
| **Death Penalty** | In enemy death handler | ~50 lines | EXP loss on death |
| **MVP System** | In enemy death handler | ~100 lines | MVP drops, announcements |

---

## Recommended Priority Updates

### Critical (accuracy < 60%)

1. **Skill_System.md** -- Rewrite completely. Add all 13 second-class skill tables. Remove "deferred" markers for implemented skills. Update counts. This is the most misleading doc.
2. **NodeJS_Server.md** -- Update all counts (events: 79, modules: 19, shops: 5, tables: 15+, ticks: 12+). Add missing socket events. Update data module table.
3. **Monster_Skill_System.md** -- Update monster count (27), mark SUMMONSLAVE and METAMORPHOSIS as IMPLEMENTED, update active monster table.

### High (accuracy 60-70%)

4. **Status_Effect_Buff_System.md** -- Update status count (12), buff count (95). Remove "Future" labels. Add missing buff categories.
5. **Card_System.md** -- Move GTB, Drake, auto-cast, status-on-hit from "Deferred" to "Implemented".
6. **API_Documentation.md** -- Fix character creation (name 2-24, always novice, hair/color/gender fields).

### Medium (accuracy 70-85%)

7. **Combat_System.md** -- Remove "simplified damage" description. Update to reflect full RO formula usage. Document ASPD potion integration. Add refine ATK.
8. **Enemy_System.md** -- Fix Overview spawn timing. Add monster skill integration. Update combat hooks.
9. **Pet_System.md** -- Update pet count (54). Minor.
10. **Inventory_System.md** -- Add inventory:merge, cart system, equipment:repair. Update line references.
11. **EXP_Leveling_System.md** -- Add party EXP distribution, death penalty.

---

## File Line Count Reference (for cross-verification)

| File | Lines | Doc Claims |
|------|-------|------------|
| `server/src/index.js` | 32,566 | "~32,200" |
| `server/src/ro_buff_system.js` | 1,179 | "1,179" (correct) |
| `server/src/ro_status_effects.js` | 711 | "711" (correct) |
| `server/src/ro_ground_effects.js` | 622 | "622" (correct) |
| `server/src/ro_monster_skills.js` | 548 | "548" (correct) |
| `server/src/ro_item_effects.js` | 520 | "520" (correct) |
| `server/src/ro_card_prefix_suffix.js` | 541 | "541" (correct) |
| `server/src/ro_homunculus_data.js` | 243 | "243" (correct) |
| `server/src/ro_arrow_crafting.js` | 77 | "77" (correct) |
| `server/src/ro_damage_formulas.js` | 1,079 | "1,079" (correct) |
