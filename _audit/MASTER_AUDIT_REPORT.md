# Sabri_MMO Master Audit Report

**Date**: 2026-03-22
**Scope**: 20 parallel agents, 4 audit categories, 5-7 passes each
**Total tokens consumed**: ~2.9M across all agents
**Files generated**: 20 detailed audit .md files

---

## Executive Summary

| Category | Agents | Critical Bugs | High Bugs | Medium | Low | Clean Areas |
|----------|--------|--------------|-----------|--------|-----|-------------|
| Test Suite Generation | 5 | 2 | 4 | 12 | 7 | — |
| Documentation Sync | 4 | 5 | 14 | 53 | 23 | Global Rules mostly accurate |
| Cross-System Integration | 6 | 8 | 11 | 16 | 5 | Element modifiers (0 violations) |
| Monster Template/AI | 5 | 7 | 4 | 5 | 3 | Drop rates accurate, stat quality high |
| **TOTAL** | **20** | **22** | **33** | **86** | **38** | |

**Grand total: 179 findings** (22 critical, 33 high, 86 medium, 38 low)

---

## Task 3: Comprehensive Test Suite Generation

### Output Files
- `_audit/tests/combat_formulas_audit.md` — 14 damage functions, 155 Jest tests
- `_audit/tests/combat_formulas_tests.md` — Ready-to-use Jest test code
- `_audit/tests/first_class_skills_audit.md` — 55 skills (7 classes), 97 test cases
- `_audit/tests/second_class_skills_A_audit.md` — 79 skills (Knight/Crusader/Wizard/Sage/Hunter), 62 handlers
- `_audit/tests/second_class_skills_B_audit.md` — 106 skills (Priest/Monk/Assassin/Rogue/Bard/Dancer/Blacksmith/Alchemist)
- `_audit/tests/systems_events_audit.md` — 81 socket events, 72+ buffs, 490+ items, 180+ tests

### Total Coverage
- **240+ skills** documented with handler line numbers and formulas
- **530+ Jest test cases** generated across all suites
- **25 potential issues** identified during test generation

### Critical/High Issues Found During Test Generation

| ID | Severity | Description | Location |
|----|----------|-------------|----------|
| T1 | **CRITICAL** | Rogue Double Strafe `executePhysicalSkillOnEnemy` arguments shifted by one — `dsZone` (string) passed as `skill` (object). Skill is completely broken at runtime. | index.js:19711 |
| T2 | **CRITICAL** | Ki Explosion + Hammer Fall pass `{ duration: N }` (object) where plain number expected for stun duration. Produces `NaN` expiry — stuns never apply. | index.js:18543, 20111 |
| T3 | HIGH | Jupitel Thunder Lex Aeterna total miscalculation | index.js:14526 |
| T4 | HIGH | Cast Cancel not recovering SP despite skill data having recovery values | Skill 809 handler |
| T5 | HIGH | Holy Cross missing `_lastDamageDealt` for MVP tracking | Skill 1301 handler |
| T6 | HIGH | Aqua Benedicta implementation incomplete | Skill 412 handler |

---

## Task 4: Documentation Sync

### Output Files
- `_audit/docsync/architecture_sync_audit.md` — 5 docs, 191 claims verified
- `_audit/docsync/client_cpp_sync_audit.md` — 17 docs vs 60+ source files
- `_audit/docsync/server_sync_audit.md` — 11 docs, 109 discrepancies
- `_audit/docsync/dev_reference_sync_audit.md` — 20 docs audited

### Documentation Accuracy by File

| Document | Accuracy | Status |
|----------|----------|--------|
| Global_Rules.md | ~95% | Current |
| System_Architecture.md | ~75% | Outdated (subsystem count, tick rates) |
| Database_Architecture.md | ~70% | Missing 6+ tables, 15+ columns |
| Multiplayer_Architecture.md | ~40% | **Severely outdated** (30/79 socket events) |
| Project_Overview.md | ~80% | Missing recent systems |
| NodeJS_Server.md | ~55% | Undercounts everything |
| Skill_System.md | ~50% | **Most outdated** — deferred items now implemented |
| API_Documentation.md | ~60% | Missing ~50 socket events |
| Status_Effect_Buff_System.md | ~60% | 9 "Future" buffs now active |
| Monster_Skill_System.md | ~55% | Claims stubs are implemented |
| Card_System.md | ~70% | 3 "Deferred" features now working |
| Combat_System.md | ~65% | Missing dual wield, refine ATK |
| Event_Reference.md | ~35% | **30 of 79 events listed** |
| Strategic_Implementation_Plan_v3.md | ~40% | Shows completed phases as NOT STARTED |
| Test_Checklist.md | ~30% | References deleted UMG widgets |
| Configuration_Reference.md | ~80% | 3 wrong constants |

### Critical Documentation Gaps

| ID | Description |
|----|-------------|
| D1 | **11 C++ subsystems completely undocumented**: Cart, Vending, Party, ItemInspect, Crafting, Targeting, Summon, Chat, Inventory, Enemy, OtherPlayer |
| D2 | **49 socket events undocumented** (entire categories: party, pet, homunculus, cart, vending, crafting) |
| D3 | **20+ server systems undocumented**: Party, Cart, Vending, Forge/Refine, Homunculus, Pharmacy, Ensemble, Abracadabra, Death Penalty, MVP rewards |
| D4 | **6 database tables undocumented**: character_memo, skills, skill_prerequisites, skill_levels, character_skills, character_pets |
| D5 | **Subsystem count inconsistent**: docs say 25/30/33, actual is 34 |

---

## Task 5: Cross-System Integration Audit

### Output Files
- `_audit/integration/zone_filtering_audit.md` — 114 iterations cataloged
- `_audit/integration/magic_rod_pneuma_audit.md` — 25 skill handlers checked
- `_audit/integration/lex_aeterna_audit.md` — 96 damage points cataloged
- `_audit/integration/card_procs_audit.md` — 17 hook functions verified
- `_audit/integration/element_modifiers_audit.md` — 80+ skills, 400 element values
- `_audit/integration/buff_interactions_audit.md` — 89 buff/debuff types

### Zone Filtering (90/114 compliant — 79%)

| ID | Severity | Violation | Line |
|----|----------|-----------|------|
| Z1 | MEDIUM | Fire Wall ground effect (enemies) — no zone filter | 27706 |
| Z2 | MEDIUM | Fire Wall ground effect (players) — no zone filter | 27806 |
| Z3 | LOW | Play Dead deaggro iterates all zones | 12578 |
| Z4 | LOW | Cloaking deaggro iterates all zones | 16823 |
| Z5 | LOW | Disconnect deaggro iterates all zones | 7397 |

### Magic Rod & Pneuma (17 violations)

**Pneuma — only 3/15 ranged skills check it:**

| ID | Severity | Skill Missing Pneuma Check |
|----|----------|---------------------------|
| P1 | HIGH | Double Strafe (303) |
| P2 | HIGH | Arrow Repel (306) |
| P3 | HIGH | Shield Boomerang (1305) |
| P4 | HIGH | Rogue Double Strafe (1704) |
| P5 | MEDIUM | Musical Strike (1500) |
| P6 | MEDIUM | Slinging Arrow (1520) |
| P7 | MEDIUM | Phantasmic Arrow (913) |
| P8 | MEDIUM | Arrow Shower (304) |
| P9 | MEDIUM | Throw Venom Knife (1711) |
| P10 | MEDIUM | Grimtooth (1107) |

**Magic Rod — missing from:**

| ID | Severity | Skill Missing Magic Rod Check |
|----|----------|------------------------------|
| MR1 | HIGH | Monster `executeMonsterPlayerSkill` magic path — monsters casting bolts never trigger Magic Rod | 29130 |
| MR2 | MEDIUM | Turn Undead |
| MR3 | LOW | Sight Blaster reactive trigger |
| MR4 | LOW | Hindsight autocast |

**Root cause**: Checks are per-handler, not centralized in damage functions.

### Lex Aeterna (31/51 eligible paths — 60.8% coverage)

**20 damage paths missing LA check:**

| ID | Severity | Skill/Path Missing LA |
|----|----------|----------------------|
| LA1 | HIGH | Bash (101) |
| LA2 | HIGH | Magnum Break (102) |
| LA3 | HIGH | Double Strafe (303) |
| LA4 | HIGH | Arrow Shower (304) |
| LA5 | HIGH | Back Stab (1701) |
| LA6 | HIGH | Raging Trifecta Blow (1602) |
| LA7 | MEDIUM | Arrow Repel (306) |
| LA8 | MEDIUM | Envenom (500) |
| LA9 | MEDIUM | Heal vs Undead |
| LA10 | MEDIUM | Turn Undead fail path |
| LA11 | MEDIUM | Ki Explosion splash |
| LA12 | MEDIUM | Raid (1703) |
| LA13 | MEDIUM | Intimidate (1707) |
| LA14 | MEDIUM | PvP auto-attack |
| LA15 | MEDIUM | Sonic Blow splash |
| LA16 | MEDIUM | Venom Splasher detonation |
| LA17 | LOW | Throw Stone |

**Positive**: No double-consumption or missed-consumption bugs found.

### Card Proc Hooks (6 critical gaps)

| ID | Severity | Description |
|----|----------|-------------|
| CP1 | **CRITICAL** | `executeMonsterPlayerSkill` — monster-cast player skills trigger ZERO when-hit procs | 29015 |
| CP2 | **CRITICAL** | `elemental_melee` NPC skill — no when-hit procs |
| CP3 | **CRITICAL** | `status_melee` NPC skill — no when-hit procs |
| CP4 | **CRITICAL** | `multi_hit` NPC skill — no when-hit procs |
| CP5 | **CRITICAL** | `forced_crit` NPC skill — no when-hit procs |
| CP6 | **CRITICAL** | `aoe_physical` + `drain_hp` NPC skills — no when-hit procs |
| CP7 | MODERATE | PvP auto-attack path has zero card hooks (all 8 missing) | 26060 |
| CP8 | MODERATE | Baphomet Card splash targets don't trigger drain/status procs |

**Impact**: Cards like Orc Hero (stun attacker on hit), Maya Purple (auto-cast on hit), Hodremlin (autobonus) never trigger from monster skill damage.

### Element Modifiers (CLEAN — 0 violations)

All 400 element table values correct. Element priority (Endow > Arrow > Weapon) correct. Pipeline order correct. All 80+ skills use correct elements and damage types.

### Buff Interactions (2 critical, 3 high, 7 medium)

| ID | Severity | Description |
|----|----------|-------------|
| B1 | **CRITICAL** | `BUFFS_SURVIVE_DEATH` uses `song_humming` but actual buff name is `dance_humming` — Humming incorrectly cleared on death | index.js:2035 |
| B2 | **CRITICAL** | `UNDISPELLABLE` uses `stripweapon/shield/armor/helm` but actual names use underscores: `strip_weapon/shield/armor/helm` — Dispel can remove active strips | index.js:15320 |
| B3 | HIGH | Provoke on player target doesn't emit `player:stats` — ATK/DEF changes invisible |
| B4 | HIGH | Decrease AGI on player doesn't emit `player:stats` — AGI/ASPD invisible |
| B5 | HIGH | Blessing cures Curse/Stone then RETURNS without applying stat buff (early return at line 11499) |
| B6 | MEDIUM | Quagmire not stripping enemy buffs |
| B7 | MEDIUM | 5 buff types missing `player:stats` emission (Angelus, Endure, IC, Quagmire, Signum Crucis) |

---

## Task 6: Monster Template/AI Audit

### Output Files
- `_audit/monsters/monster_stats_batch1_audit.md` — IDs 1001-1185 (170 monsters)
- `_audit/monsters/monster_stats_batch2_audit.md` — IDs 1101-1299 (170 monsters)
- `_audit/monsters/monster_stats_batch3_audit.md` — IDs 1300-2068 (169 monsters, all MVPs)
- `_audit/monsters/monster_ai_behavior_audit.md` — 18 AI types, mode flags, tick loop
- `_audit/monsters/monster_skills_drops_audit.md` — 27/509 skill coverage, drop tables

### Monster Stats (509 templates total)

**Overall quality: HIGH** — auto-generated from rAthena, core stats (HP/ATK/DEF/MDEF/Level/Size/Race) accurate.

**6 Critical Element Errors:**

| Monster | ID | Current | Correct | Impact |
|---------|-----|---------|---------|--------|
| Golem | 1040 | Neutral **Lv1** | Neutral **Lv3** | Element damage multipliers wrong |
| Orc Skeleton | 1152 | **Neutral** 1 | **Undead** 1 | Holy deals 100% instead of 150% |
| Orc Zombie | 1153 | **Neutral** 1 | **Undead** 1 | Holy deals 100% instead of 150% |
| Kobold | 1133 | **Neutral** 1 | **Wind** 2 | Earth should deal 150% |
| Orc Archer | 1189 | **Neutral** 1 | **Earth** 1 | Fire/Wind should deal 150% |
| High Orc | 1213 | **Neutral** 1 | **Fire** 2 | Water should deal 150% |

**Root cause (Batch 2)**: rAthena YAML parser likely defaulted to Neutral 1 when element was inside a `raceGroups` override block.

**Other Issues:**
- Baphomet Jr. (1101) drops "Baphomet Card" instead of "Baphomet Jr. Card"
- Giant Whisper (1186) has `monsterClass: 'normal'` should be `'boss'`
- 21+ MVPs missing 1-2 MVP drop slots (rAthena provides 3 per MVP)
- 3 MVP item name errors: Dracula (1carat→3carat Diamond), Orc Hero (Red Jewel→Steel), Doppelganger (Ruby→Cursed Ruby)
- 299 monsters have `str: 0` vs rAthena default of `1` (negligible impact)
- All MVPs use flat 1-hour respawn (RO Classic has variable windows)
- 6 late-episode MVPs missing (acceptable for classic scope)

### Monster AI (18 AI types, 14/18 mode flags implemented)

| ID | Severity | Bug | Impact |
|----|----------|-----|--------|
| AI1 | **CRITICAL** | AI type 04 "Angry" re-aggro not implemented | ~200 monsters (Zombie, Munak, Ghoul) trivially kitable |
| AI2 | HIGH | AI type 01 flee-on-hit not implemented | ~15 monsters (Fabre, Lunatic) fight back instead of fleeing |
| AI3 | HIGH | AI type 06 plant 1-damage cap not implemented | ~100 plants/eggs/chests take full damage |
| AI4 | HIGH | CastSensorChase only in IDLE state, not CHASE | ~380 monsters don't switch targets mid-chase on cast |
| AI5 | MEDIUM | No monster return-to-spawn after leash distance exceeded | Monsters chase indefinitely |
| AI6 | MEDIUM | No link aggro range limit | Same-type assist has no distance check |

**10 missing behaviors**: Angry re-aggro, flee-on-hit, plant cap, CastSensor mid-chase, return-to-spawn, link range, change target on HP threshold, random walk variety, dead branch summoning AI, monster body block.

### Monster Skills (5.3% coverage)

| Metric | Current | RO Classic (rAthena) | Gap |
|--------|---------|---------------------|-----|
| Monsters with skills | 27 | ~281 | 254 missing |
| NPC_ skill types implemented | 17 | ~47 | 30 missing |
| Average MVP skills | 3-4 | 12-16 | ~28% coverage |

| ID | Severity | Issue |
|----|----------|-------|
| MS1 | **CRITICAL** | Slave monsters killed by players give NO EXP/drops. In RO Classic, player-killed slaves give normal rewards; only master-death kills suppress rewards. | index.js:2167-2178 |
| MS2 | HIGH | NPC_CALLSLAVE (re-summon dead slaves) not implemented — used by 17+ MVPs |
| MS3 | HIGH | NPC_POWERUP (+200% ATK buff) not implemented — 15+ MVPs |
| MS4 | HIGH | NPC_GUIDEDATTACK (always-hit) not implemented — 12+ MVPs |
| MS5 | MEDIUM | 5 monsters have wrong skill assignments (Hornet, Zombie, Skeleton, Spore, Smokie) |

---

## Top 10 Most Impactful Fixes

These are the findings that would have the largest gameplay impact if fixed:

| Rank | ID | Category | Description |
|------|-----|----------|-------------|
| 1 | T1 | Skills | **Rogue Double Strafe completely broken** — argument shift |
| 2 | CP1-6 | Cards | **When-hit card procs never trigger from monster skills** — Orc Hero, Maya Purple, etc. useless |
| 3 | AI1 | Monsters | **200 monsters trivially kitable** — Angry re-aggro missing |
| 4 | B1-B2 | Buffs | **BUFFS_SURVIVE_DEATH and UNDISPELLABLE naming mismatches** — wrong buff names |
| 5 | MS1 | Monsters | **Slave monsters give no EXP/drops** — MVPs less rewarding |
| 6 | B5 | Buffs | **Blessing doesn't apply stat buff after curing** — early return |
| 7 | AI3 | Monsters | **Plants/eggs take full damage** — should always be 1 |
| 8 | T2 | Skills | **Ki Explosion + Hammer Fall stuns never apply** — NaN duration |
| 9 | P1-P4 | Pneuma | **12 ranged skills bypass Pneuma** — defensive skill ineffective |
| 10 | LA1-LA6 | Lex Aeterna | **20 skills don't double damage with LA** — Priest synergy broken |

---

## Files Generated

```
_audit/
├── MASTER_AUDIT_REPORT.md          ← This file
├── tests/
│   ├── combat_formulas_audit.md     (damage pipeline analysis + 155 Jest tests)
│   ├── combat_formulas_tests.md     (Jest code ready to save as .test.js)
│   ├── first_class_skills_audit.md  (55 skills, 97 test cases)
│   ├── second_class_skills_A_audit.md (79 skills — Knight/Crusader/Wizard/Sage/Hunter)
│   ├── second_class_skills_B_audit.md (106 skills — Priest/Monk/Assassin/Rogue/Bard/Dancer/BS/Alch)
│   └── systems_events_audit.md      (81 socket events, 72 buffs, 490 items, 180 tests)
├── docsync/
│   ├── architecture_sync_audit.md   (5 docs, 191 claims, 34 inaccuracies)
│   ├── client_cpp_sync_audit.md     (17 docs, 47 issues, 11 undocumented subsystems)
│   ├── server_sync_audit.md         (11 docs, 109 discrepancies)
│   └── dev_reference_sync_audit.md  (20 docs, 10 outdated, 5 obsolete)
├── integration/
│   ├── zone_filtering_audit.md      (114 iterations, 4 violations)
│   ├── magic_rod_pneuma_audit.md    (17 violations across 25 skills)
│   ├── lex_aeterna_audit.md         (96 damage points, 20 missing checks)
│   ├── card_procs_audit.md          (6 critical gaps in monster skill paths)
│   ├── element_modifiers_audit.md   (0 violations — CLEAN)
│   └── buff_interactions_audit.md   (2 critical, 3 high, 7 medium bugs)
└── monsters/
    ├── monster_stats_batch1_audit.md (170 monsters, 3 element errors)
    ├── monster_stats_batch2_audit.md (170 monsters, 3 element errors, 1 card name)
    ├── monster_stats_batch3_audit.md (169 monsters, MVP drop gaps, 3 item errors)
    ├── monster_ai_behavior_audit.md  (18 AI types, 10 bugs, 10 missing behaviors)
    └── monster_skills_drops_audit.md (27/509 skill coverage, slave EXP bug)
```

---

*Report compiled from 20 parallel agent audits. Each agent performed 5-7 passes through the codebase. Total: ~2.9M tokens consumed.*
