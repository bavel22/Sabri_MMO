# Sabri_MMO Master Audit Report — Round 2

**Date**: 2026-03-23
**Scope**: 20 parallel agents — verification, security, performance, code quality, deep-dive fix specs
**Purpose**: Verify Round 1 findings, expand to new audit areas, produce actionable fix specifications

---

## Executive Summary

| Category | Agents | Critical | High | Medium | Low |
|----------|--------|----------|------|--------|-----|
| Verification (Round 1 bugs) | 2 | 0 new | 1 upgraded | 2 denied | 0 |
| Security | 4 | 11 | 18 | 22 | 13 |
| Performance | 2 | 2 | 5 | 4 | 3 |
| Code Quality | 4 | 5 | 13 | 13 | 10 |
| Fix Specifications | 6 | 3 new | 5 new | 6 new | 0 |
| **TOTAL** | **20** | **21** | **42** | **47** | **26** |

**Round 2 grand total: 136 new findings** (plus 6 detailed fix specs with exact code changes)

**Combined R1+R2 total: 315 findings** (43 critical, 75 high, 133 medium, 64 low)

---

## Round 1 Verification Results

**8 of 10 critical bugs CONFIRMED**, 2 DENIED:

| Bug | Status | Notes |
|-----|--------|-------|
| T1 — Rogue Double Strafe args | **CONFIRMED** | Arguments shifted, skill non-functional |
| T2 — Ki Explosion stun duration | **CONFIRMED + UPGRADED** | Stuns are PERMANENT (NaN expiry), not "never applied" |
| T2b — Hammer Fall stun duration | **CONFIRMED + UPGRADED** | Same — permanent stuns |
| B1 — BUFFS_SURVIVE_DEATH `song_humming` | **CONFIRMED** | Should be `dance_humming` |
| B2 — UNDISPELLABLE strip names | **CONFIRMED** | `stripweapon` vs `strip_weapon` |
| B5 — Blessing early return | **DENIED** | Correct RO Classic behavior (cure-only) |
| CP1 — Monster skill when-hit procs | **CONFIRMED** | Zero procs in all monster skill paths |
| MS1 — Slave monster no EXP | **DENIED** | Correct per rAthena pre-renewal |
| AI1 — Angry re-aggro | **CONFIRMED** | AI type 04 check exists but never used |
| AI3 — Plant 1-damage cap | **CONFIRMED** | No plant mode handling in damage pipeline |

**Monster elements: 8 confirmed** (6 from R1 + Orc Lady added + Baphomet Jr. card name). 6 additional monsters checked were already correct.

---

## Security Audit (4 agents)

### Output Files
- `security_sql_injection_audit.md` — 378 queries audited
- `security_auth_validation_audit.md` — JWT, routes, input validation
- `security_socket_abuse_audit.md` — 80 socket events assessed
- `rest_api_full_audit.md` — 18 endpoints audited

### SQL Injection: **CLEAN** (377/378 parameterized)
One `Function()` constructor for card script math (low risk, sanitized input).

### Top Security Findings

| ID | Severity | Category | Description |
|----|----------|----------|-------------|
| S1 | **CRITICAL** | Socket | `player:position` accepts client-supplied `characterId` — can move ANY player |
| S2 | **CRITICAL** | Socket | No position distance/speed validation — teleport hack |
| S3 | **CRITICAL** | Socket | `_castComplete: true` in `skill:use` bypasses all cast times |
| S4 | **CRITICAL** | Socket | `combat:attack` has no zone check for enemy targets |
| S5 | **CRITICAL** | Socket | `vending:buy` no DB transaction — item duplication via race |
| S6 | **CRITICAL** | Socket | `inventory:use` + `inventory:drop` race — concurrent exploitation |
| S7 | **CRITICAL** | Socket | No auth on pre-`player:join` events |
| S8 | HIGH | Auth | No per-account brute force protection on login |
| S9 | HIGH | API | Test mode endpoints always mounted (no env gate) |
| S10 | HIGH | CORS | `origin: "*"` on both Express and Socket.io |
| S11 | HIGH | API | `GET/PUT /api/characters/:id` missing `deleted = FALSE` filter |
| S12 | HIGH | Socket | No range check on targeted skills |
| S13 | HIGH | Socket | 45 of 80 events lack rate limiting |
| S14 | HIGH | Socket | `identify:select` no skill/item requirement check |
| S15 | HIGH | API | No `helmet` middleware |
| S16 | HIGH | Socket | Chat not HTML-sanitized |

### Security Strengths
- Server-authoritative damage/stats/economy
- All SQL parameterized (zero injection risk)
- bcrypt password hashing
- Character ownership verification on all endpoints
- 37 events already rate-limited

---

## Performance Audit (2 agents)

### Output Files
- `performance_tick_memory_audit.md` — All tick loops, memory, scalability
- `performance_database_audit.md` — 400 queries, indexes, N+1 patterns

### Top Performance Findings

| ID | Severity | Description |
|----|----------|-------------|
| P1 | **CRITICAL** | `await getPlayerPosition()` in 50ms combat tick — redundant Redis call (data in memory) |
| P2 | **CRITICAL** | No `max` on PostgreSQL pool (defaults to 10, exhausts under load) |
| P3 | HIGH | Enemy AI aggro scan O(enemies × ALL players) — no zone filtering |
| P4 | HIGH | 12+ sequential queries on player join (could be 2-3) |
| P5 | HIGH | N+1 periodic save: 3 queries × N players every 60s |
| P6 | HIGH | `getPlayerInventory()` JOINs 50 item columns (already cached in memory) |
| P7 | HIGH | Missing composite index: `character_inventory(character_id, is_equipped)` |

### Scalability Projection
| Players | Status |
|---------|--------|
| 10-30 | Works fine |
| 50-100 | Combat tick overruns 50ms budget |
| 100-200 | Multiple tick loops break down |
| 500+ | Global entity iteration without spatial partitioning fails |

### Memory: Largely clean
- `afterCastDelayEnd` Map grows unboundedly (slow)
- No socket listener leaks
- All DB transactions properly released

---

## Code Quality (4 agents)

### Output Files
- `code_quality_dead_code_audit.md` — ~12k lines dead/dormant
- `code_quality_error_handling_audit.md` — Process-level crash risks
- `code_quality_race_conditions_audit.md` — 23 race conditions
- `code_quality_consistency_audit.md` — Patterns, naming, duplication

### Top Code Quality Findings

| ID | Severity | Category | Description |
|----|----------|----------|-------------|
| CQ1 | **CRITICAL** | Error | Zero `uncaughtException`/`unhandledRejection` handlers — server crashes on any unhandled throw |
| CQ2 | **CRITICAL** | Error | No `pool.on('error')` — idle PostgreSQL connection drop is fatal |
| CQ3 | **CRITICAL** | Error | 8 async socket handlers without try/catch (including `player:position` at 30Hz and `skill:use`) |
| CQ4 | **CRITICAL** | Race | Double-death: no `isDead` guard at entry to `processEnemyDeathFromSkill` — double EXP/drops |
| CQ5 | **CRITICAL** | Dead Code | `calculateASPD(player)` called in `recalcPetBonuses()` but function doesn't exist — ReferenceError crash |
| CQ6 | HIGH | Race | `addItemToInventory` read-then-write on quantity (not atomic) |
| CQ7 | HIGH | Race | Vending zeny check without transaction |
| CQ8 | HIGH | Race | SP check-then-deduct gap on instant skills |
| CQ9 | HIGH | Error | 17 setInterval tick loops with zero try/catch protection |
| CQ10 | HIGH | Error | All Redis helper functions have no try/catch |
| CQ11 | HIGH | Consistency | 16 handlers missing `skill:buff_removed` emit after damage break |
| CQ12 | HIGH | Consistency | Damage-apply-broadcast-death sequence copy-pasted ~80 times |
| CQ13 | HIGH | Dead Code | 8 dead functions (~89 lines), 6 unused imports, 3 unused data files (~11.5k lines) |

### Race Condition Summary
- 4 CRITICAL, 7 HIGH, 7 MEDIUM, 4 LOW
- **Fix #1** (1 line): `if (enemy.isDead) return;` at top of `processEnemyDeathFromSkill`
- **Fix #2**: Atomic `UPDATE ... SET quantity = quantity + $1 RETURNING` for inventory
- **Fix #3**: DB transaction for `vending:buy`
- **Fix #4**: Conditional `UPDATE ... WHERE amount >= $1` for vending stock

---

## Deep-Dive Fix Specifications (6 agents)

### Output Files
- `fix_spec_pneuma_magicrod.md` — 10 Pneuma + 1 Magic Rod fix
- `fix_spec_lex_aeterna.md` — 17 LA fixes with exact code
- `fix_spec_card_procs.md` — 8 function fixes + death check gaps
- `fix_spec_buff_naming.md` — 15 naming mismatches + client bug
- `fix_spec_monster_ai.md` — 4 AI behavior implementations
- (verification files count as fix guidance too)

### Pneuma Fix Spec
- **10 skills need fixes** (not 12 — Grimtooth is melee splash, Blitz Beat is MISC)
- 4 skills: just add `isRanged: true` to existing `executePhysicalSkillOnEnemy` call
- 6 skills: inline Pneuma check before damage calculation
- Magic Rod: one insertion in `executeMonsterPlayerSkill` magic path

### Lex Aeterna Fix Spec
- **17 actual fixes** (Sonic Blow was false positive — already has LA)
- Individual inline checks matching existing 40+ LA check pattern
- Multi-hit skills (Back Stab, Triple Attack) need `lexConsumed` flag
- Triple Attack has separate design bug (fires after auto-attack, should replace it)

### Card Proc Fix Spec
- 7 functions need when-hit hooks (executeMonsterPlayerSkill + 6 NPC skills)
- **Bonus**: 5 of 6 NPC skill types have NO death check — players become zombies at 0 HP
- PvP auto-attack path needs all 8 hook categories

### Buff Naming Fix Spec — 15 mismatches found
| Severity | Count | Key Issues |
|----------|-------|------------|
| CRITICAL | 2 | `song_humming`→`dance_humming`, `stripweapon`→`strip_weapon` (4 entries) |
| HIGH | 5 | Phantom `cp_*` entries, phantom `combo`/`dancing`, unimplemented names, **Abracadabra endow wrong names** (`endow_blaze`→`endow_fire` etc.) |
| MEDIUM | 6 | 11 buffs missing from BUFF_TYPES, `armor_break`/`weapon_break` have no stat effects |
| CLIENT | 2 | `BuffBarSubsystem` strips underscores but `PlayerInputSubsystem::HasBuff()` checks WITH underscores — always fails; two UNDISPELLABLE sets out of sync |

### Monster AI Fix Spec — 4 implementations
| Fix | Monsters | Lines to Add |
|-----|----------|-------------|
| Angry re-aggro | 233 | ~15 lines after chase give-up |
| Cowardly flee | 44 | ~80 lines (new FLEE state) |
| Plant 1-damage cap | 115 | ~20 lines at damage chokepoints |
| CastSensor mid-chase | 316 | ~15 lines in CHASE state |

### Database Schema Audit — 6 critical issues
| ID | Description |
|----|-------------|
| DB1 | 3 FKs reference `characters(id)` but PK is `character_id` — CREATE fails |
| DB2 | `character_hotbar.skill_level` queried but never created |
| DB3 | `refine_level`/`compounded_cards` not in auto-creation block |
| DB4 | `characters.deleted` not in auto-creation — soft-delete breaks |
| DB5 | `characters.class` defaults to `'warrior'` should be `'novice'` |
| DB6 | Character name uniqueness doesn't filter `deleted = FALSE` |

### Client Subsystem Audit — **CLEAN**
- All 34 subsystems: correct lifecycle, UPROPERTY compliance, null guards
- 88 unique socket events across 176 registrations
- 96/171 skill VFX configs (56% — passives don't need VFX)
- 7 Z-order collisions (LOW, never simultaneous)

---

## Combined Priority Fix List (R1 + R2)

### P0 — Fix Immediately (Server Crash / Security Exploit)

| # | Finding | Fix Effort |
|---|---------|-----------|
| 1 | `uncaughtException` + `unhandledRejection` handlers | 10 lines |
| 2 | `pool.on('error')` handler | 5 lines |
| 3 | `player:position` use `findPlayerBySocketId()` not client characterId | 1 line |
| 4 | `calculateASPD` ReferenceError in `recalcPetBonuses()` | 1 line |
| 5 | `if (enemy.isDead) return` in `processEnemyDeathFromSkill` | 1 line |
| 6 | try/catch on `player:position` and `skill:use` handlers | 6 lines each |
| 7 | Remove `_castComplete` trust from `skill:use` | 3 lines |
| 8 | Add zone check to `combat:attack` | 1 line |

### P1 — Fix Soon (Gameplay Bugs / Data Integrity)

| # | Finding | Fix Effort |
|---|---------|-----------|
| 9 | Rogue Double Strafe argument order | 1 line |
| 10 | Ki Explosion / Hammer Fall stun duration (object→number) | 2 lines |
| 11 | BUFFS_SURVIVE_DEATH `song_humming`→`dance_humming` | 1 line |
| 12 | UNDISPELLABLE strip names (add underscores) | 4 lines |
| 13 | Abracadabra endow names (`endow_blaze`→`endow_fire` etc.) | 4 lines |
| 14 | NPC skill death checks (5 functions) | 25 lines |
| 15 | When-hit card procs in monster skill paths | 30 lines |
| 16 | 8 monster element fixes | 8 lines |
| 17 | Baphomet Jr. card drop name | 1 line |
| 18 | Vending buy DB transaction | 10 lines |
| 19 | BuffBarSubsystem underscore handling | 1 line in C++ |
| 20 | 3 FK references to wrong column | 3 lines in migrations |

### P2 — Fix When Possible (Gameplay Polish)

| # | Finding | Fix Effort |
|---|---------|-----------|
| 21 | 10 Pneuma checks on ranged skills | 30-50 lines |
| 22 | 17 Lex Aeterna checks | 50-70 lines |
| 23 | Monster AI: Angry re-aggro + Plant cap + CastSensor | ~50 lines |
| 24 | Monster AI: Cowardly flee | ~80 lines |
| 25 | Redundant Redis call in combat tick | 1 line removal |
| 26 | PostgreSQL pool max configuration | 1 line |
| 27 | Missing composite index | 1 SQL statement |
| 28 | Rate limiting on 45 unprotected events | ~45 lines |
| 29 | CORS restriction | 2 lines |
| 30 | `helmet` middleware | 2 lines |

---

## All Round 2 Files Generated

```
_audit/
├── MASTER_AUDIT_REPORT.md              (Round 1)
├── MASTER_AUDIT_REPORT_R2.md           (This file — Round 2)
├── verify_critical_bugs.md             (10 bugs verified)
├── verify_monster_elements.md          (14 monsters checked)
├── security_sql_injection_audit.md     (378 queries — CLEAN)
├── security_auth_validation_audit.md   (JWT, routes, validation)
├── security_socket_abuse_audit.md      (80 events assessed)
├── rest_api_full_audit.md              (18 endpoints)
├── performance_tick_memory_audit.md    (tick loops, memory, scalability)
├── performance_database_audit.md       (400 queries, indexes)
├── code_quality_dead_code_audit.md     (~12k dead lines)
├── code_quality_error_handling_audit.md (crash risks)
├── code_quality_race_conditions_audit.md (23 race conditions)
├── code_quality_consistency_audit.md   (patterns, naming)
├── fix_spec_pneuma_magicrod.md         (10+1 fixes with code)
├── fix_spec_lex_aeterna.md             (17 fixes with code)
├── fix_spec_card_procs.md              (8 function fixes + death checks)
├── fix_spec_buff_naming.md             (15 naming mismatches)
├── fix_spec_monster_ai.md              (4 AI implementations)
├── database_schema_full_audit.md       (17 tables, 25 migrations)
├── client_subsystem_full_audit.md      (34 subsystems — CLEAN)
├── tests/                              (Round 1 — 530+ test cases)
├── docsync/                            (Round 1 — 4 doc sync audits)
├── integration/                        (Round 1 — 6 integration audits)
└── monsters/                           (Round 1 — 5 monster audits)
```

**Total audit files: 37** across Rounds 1 and 2.

---

*Round 2 compiled from 20 parallel agent audits. Combined with Round 1: 40 agents, ~5.5M tokens, 37 audit files, 315 findings.*
