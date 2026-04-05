# Sage Skills Comprehensive Audit — RO Classic Pre-Renewal Compliance

**Date:** 2026-03-16 (v2 — deep research verified)
**Scope:** All 22 Sage skills (IDs 1400-1421)
**Sources:** rAthena pre-renewal `skill_db.yml`, `skill_cast_db.txt`, `skill_require_db.txt`, `status.cpp`, `skill.cpp`; iRO Wiki Classic; RateMyServer; Divine Pride; pservero pre-renewal DB
**Files Audited:** `server/src/index.js`, `server/src/ro_skill_data_2nd.js`, `server/src/ro_buff_system.js`, `server/src/ro_damage_formulas.js`, `server/src/ro_ground_effects.js`

---

## Executive Summary

- **22 skills total**: 19 implemented (16 active + 3 passive), 3 stubs (deferred)
- **25 bugs found**: 12 critical, 8 moderate, 5 data-only
- **3 deferred stubs**: Abracadabra, Create Elemental Converter, Elemental Change
- v2 research revealed 9 additional bugs not in original audit, corrected 3 original findings

---

## Skill-by-Skill Audit

### 1. Advanced Book (1400) — Passive

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| ATK bonus | +3/lv with Book weapons (flat mastery) | +3/lv with Books (line 566-570) | CORRECT |
| ASPD bonus | +0.5%/lv with Book weapons (equipment ASPD category) | NOT IMPLEMENTED | BUG #1 |
| Weapon check | Only applies with Book-type weapons | `wType === 'book'` check | CORRECT |

**Source:** divine-pride Skill 274, RateMyServer, iRO Wiki ASPD page. The ASPD bonus is classified as "Equipment ASPD" — a percentage-based reduction to attack delay applied when wielding W_BOOK. Formula in rAthena: `(skill_lv - 1) / 2 + 1`.

**BUG #1 — Advanced Book ASPD bonus missing**
- **Severity:** MODERATE
- **Location:** `getPassiveSkillBonuses()` line 566-570
- **Fix:** Add `bonuses.bookAspdBonus = abLv * 0.5` percentage, then apply as equipment ASPD in `calculateASPD()`.

---

### 2. Cast Cancel (1401) — Active/Self

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Cancel own cast | Remove active cast | Removes from `activeCasts` Map | CORRECT |
| SP cost | 2 all levels | 2 (from skill data) | CORRECT |
| Cast time | Instant (0ms) | 0ms | CORRECT |
| After-cast delay | 0ms | 0ms | CORRECT |
| SP refund | [10,30,50,70,90]% of cancelled spell's SP | NOT IMPLEMENTED | NOTE |

**Source:** rAthena pre-re DB (SP 2, cast 0, ACD 0), iRO Wiki Classic (SP retained [10,30,50,70,90]%), Divine Pride.

**NOTE — SP refund (design deviation):** In RO, SP is consumed at cast START. Cast Cancel refunds a %. In our game, SP is consumed at cast COMPLETION. Canceling means SP was never consumed. No fix needed unless SP model changes.

---

### 3. Hindsight (1402) — Active/Self Buff

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Duration | (90+lv*30)s = 120,150,...,390s | Handler: (90+lv*30)s = CORRECT | CORRECT (handler) |
| Duration (skill data) | 120000+i*30000 | `60000+i*30000` = 60,90,...,330s | BUG #2 |
| Autocast chance (pre-re) | [7,9,11,13,15,17,19,21,23,25]% | [7,9,11,13,15,17,20,22,23,25]% | BUG #3 |
| Spell pool (pre-re) | Per-level unlock: Napalm→Bolts→SS→FB→FD | Renewal groups (Bolts/SS+FB/ES+FD/TS+HD) | BUG #4 |
| Bolt level dist | 50% Lv1, 35% Lv2, 15% Lv3 | Exact match | CORRECT |
| SP cost of autocast | 66% of normal | `Math.floor(baseSP * 2 / 3)` | CORRECT |
| Cast time of autocast | None | Direct damage, no activeCasts | CORRECT |
| ACD of autocast | Yes (does NOT block further autocast triggers) | Not enforced | MINOR |
| Undispellable | Yes (confirmed iRO Classic Dispell page) | In UNDISPELLABLE set | CORRECT |
| SP cost | 35 all levels | 35 | CORRECT |
| Cast time | 3000ms | 3000ms | CORRECT |

**Source:** rAthena `skill_cast_db.txt` line 279 (`120000:150000:....:390000`), iRO Wiki Classic Hindsight page, RateMyServer Pre-Re column.

**BUG #2 — Hindsight duration wrong in skill data**
- **Severity:** MODERATE (handler is correct, data used for display only)
- **Location:** `ro_skill_data_2nd.js` line 61
- **Issue:** `duration: 60000+i*30000` gives 60s,90s,...,330s. Should be `120000+i*30000` for 120s,...,390s.
- **Fix:** Change to `duration: 120000+i*30000`.

**BUG #3 — Hindsight autocast chance minor discrepancy**
- **Severity:** LOW
- **Location:** `index.js` line 11576
- **Issue:** Current: [7,9,11,13,15,17,**20,22**,23,25] (RateMyServer). iRO Classic linear: [7,9,11,13,15,17,**19,21**,23,25] (+2%/lv). Lv7-8 differ by 1%.
- **Fix:** Change to [7,9,11,13,15,17,19,21,23,25] for cleaner pre-renewal pattern. Both sources are valid.

**BUG #4 — Hindsight spell pool uses RENEWAL groups instead of pre-renewal per-level unlock** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** `index.js` lines 17947-18014 (auto-attack tick Hindsight section)
- **Issue:** Current code uses Renewal spell groups (Lv1+: bolts, Lv4+: Soul Strike+Fire Ball, Lv7+: Earth Spike+Frost Diver, Lv10: Thunderstorm+Heaven's Drive). Pre-renewal uses per-level unlock:

| Hindsight Lv | Spell Added | Max Spell Level |
|-------------|-------------|-----------------|
| 1 | Napalm Beat | Lv3 |
| 2 | Cold Bolt, Fire Bolt, Lightning Bolt | Lv1 |
| 3 | (same bolts) | Lv2 |
| 4 | (same bolts) | Lv3 |
| 5 | Soul Strike | Lv1 |
| 6 | (same) | Lv2 |
| 7 | (same) | Lv3 |
| 8 | Fire Ball | Lv1 |
| 9 | (same) | Lv2 |
| 10 | Frost Diver | Lv1 |

**Key differences from current code:**
1. **Napalm Beat** (ID 209) should be in the pool (missing entirely)
2. **Earth Spike** (804/1417) should NOT be in pool (Renewal only)
3. **Thunderstorm** (212) should NOT be in pool (Renewal only)
4. **Heaven's Drive** (805/1418) should NOT be in pool (Renewal only)
5. **Bolt level** is capped by Hindsight level, not just by learned level
- **Fix:** Rewrite spell pool to use pre-renewal per-level table. At Lv10, pool = [Napalm Beat Lv1-3, Bolts Lv1-3, Soul Strike Lv1-3, Fire Ball Lv1-2, Frost Diver Lv1]. All capped by player's learned level.

---

### 4. Dispell (1403) — Active/Single Target

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Success rate | [60,70,80,90,100]% (50+lv*10) | effectVal = [60,70,80,90,100] | CORRECT |
| SP cost | 1 | 1 | CORRECT |
| Cast time | 2000ms (variable, DEX-reducible) | 2000ms | CORRECT |
| After-cast delay | 0ms | 0ms | CORRECT |
| Cooldown | **0ms** | **2000ms** | BUG #5 |
| Catalyst | 1 Yellow Gemstone (715) | In SKILL_CATALYSTS | CORRECT |
| Range | 9 cells (450 UE) | 450 | CORRECT |
| UNDISPELLABLE set | Extensive list (see below) | 4 entries only | BUG #6 |
| Endow revert | Revert weaponElement | Checks `startsWith('endow_')` | CORRECT |

**Source:** rAthena `skill_cast_db.txt` (ACD 0, cooldown 0), rAthena GitHub Issue #8510 (MDEF has no effect), iRO Classic Wiki Dispell page.

**BUG #5 — Dispell cooldown should be 0**
- **Severity:** MODERATE
- **Location:** `ro_skill_data_2nd.js` line 62
- **Issue:** `cooldown: 2000` — rAthena confirms cooldown is 0. Remove the cooldown.
- **Fix:** Change to `cooldown: 0`.

**BUG #6 — Dispell UNDISPELLABLE set incomplete**
- **Severity:** MODERATE
- **Location:** `index.js` line 11458
- **Issue:** Current set: `['hindsight', 'play_dead', 'auto_berserk', 'devotion_protection']`. Per rAthena source `skill.cpp` SA_DISPELL handler, these should also be undispellable:
  - Strip effects: `stripweapon`, `stripshield`, `striparmor`, `striphelm`
  - Chemical Protection: `cp_weapon`, `cp_shield`, `cp_armor`, `cp_helm`
  - Stat foods: `strfood`, `agifood`, `vitfood`, `intfood`, `dexfood`, `lukfood`
  - Combat foods: `hitfood`, `fleefood`, `batkfood`, `watkfood`, `matkfood`, `crifood`
  - Other: `enchant_deadly_poison`, `cart_boost`, `meltdown`, `safety_wall`, `dancing`, `combo`
- **Fix:** Add these buff names to the UNDISPELLABLE set as each system is implemented. Immediately add: `steel_body`, strip effects, chemical protection.

---

### 5. Magic Rod (1404) — Active/Self

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | 2 | 2 | CORRECT |
| Cast time | Instant | 0ms | CORRECT |
| Duration | [400,600,800,1000,1200]ms | Handler: `200+lv*200` = CORRECT | CORRECT (handler) |
| Duration (skill data) | [400,600,800,1000,1200]ms | `200+i*200` = [200,400,600,800,1000] | BUG #7 |
| SP absorption | [20,40,60,80,100]% of spell SP | Stored as `absorbPct` | CORRECT (stored) |
| Absorption in damage paths | Block damage + restore SP | NEVER CHECKED | BUG #8 |
| Multiple absorptions | Yes, during active window | N/A (nothing works) | BUG #8 |
| Movement cancels | Yes, moving ends absorption | NOT IMPLEMENTED | BUG #8 |
| Spell Breaker counter | SB fails, caster loses 20% MaxSP to MR user | NOT IMPLEMENTED | BUG #9 |

**Source:** iRO Wiki Magic Rod, iRO Wiki Classic Magic Rod, RateMyServer, divine-pride Skill 276.

**BUG #7 — Magic Rod duration data off-by-one**
- **Severity:** LOW (handler is correct)
- **Location:** `ro_skill_data_2nd.js` line 63
- **Fix:** Change `duration: 200+i*200` to `duration: 400+i*200`.

**BUG #8 — Magic Rod absorption not implemented**
- **Severity:** CRITICAL
- **Location:** All single-target magic damage paths in `index.js`
- **Issue:** The buff is applied but NO damage path checks for `magic_rod`. Key mechanics: absorbs single-target magic (bolts, Soul Strike, Frost Diver), restores SP, can absorb multiple spells during window, movement cancels it.
- **Fix:** In single-target magic damage handlers, check target for `magic_rod` buff. If active: nullify damage, restore SP, emit absorption event. On `player:position`, cancel `magic_rod` buff if active.

**BUG #9 — Magic Rod vs Spell Breaker counter not implemented**
- **Severity:** MODERATE
- **Location:** Spell Breaker handler, `index.js` line 11513-11568
- **Issue:** When Spell Breaker targets a Magic Rod user, SB should FAIL and the SB caster loses 20% MaxSP to the MR user.
- **Fix:** Add Magic Rod check at the start of the Spell Breaker handler.

---

### 6. Free Cast (1405) — Passive

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Movement during cast | Don't interrupt on move | Skips `interruptCast()` | CORRECT |
| Move speed while casting | [30,35,40,45,50,55,60,65,70,75]% | Full speed (no reduction) | BUG #10 |
| ASPD while casting | [55,60,65,70,75,80,85,90,95,100]% | NOT IMPLEMENTED | BUG #11 |
| Damage interrupts | Yes (Free Cast does NOT protect) | Separate system | CORRECT |

**Source:** iRO Wiki Free Cast, RateMyServer. Movement: `25 + 5*lv`%. ASPD: `50 + 5*lv`%.

**BUG #10 — Free Cast movement speed not reduced during casting**
- **Severity:** MODERATE
- **Fix:** Apply speed multiplier when player is in `activeCasts` and has Free Cast.

**BUG #11 — Free Cast ASPD while casting not implemented**
- **Severity:** LOW (only matters if auto-attacking while casting, which is rare)

---

### 7. Spell Breaker (1406) — Active/Single Target

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | 10 | 10 | CORRECT |
| Cast time | 700ms | 700ms | CORRECT |
| SP absorption | [0,25,50,75,100]% | Correct array | CORRECT |
| Lv5 HP damage | 2% target MaxHP | `Math.floor(maxHealth * 0.02)` | CORRECT |
| Lv5 HP drain | 1% target MaxHP to caster | `Math.floor(maxHealth * 0.01)` | CORRECT |
| Boss resistance | 10% success rate | `Math.random() < 0.10` | CORRECT |
| Lv5 dmg on boss resist | Applies even if interrupt fails | Outside boss check | CORRECT |
| Target not casting | Skill fails (SP consumed, nothing happens) | Lv5 damage still fires | BUG #12 |
| Magic Rod counter | SB fails, caster loses 20% MaxSP | NOT IMPLEMENTED | BUG #9 |

**Source:** rAthena `skill.cpp` SA_SPELLBREAKER handler. Confirmed Lv5 HP damage is inside `if (ud && ud->skillid)` check (target must be casting). iRO Wiki Classic, RateMyServer.

**BUG #12 — Spell Breaker Lv5 damage fires even when target isn't casting**
- **Severity:** MODERATE
- **Location:** `index.js` lines 11541-11564
- **Issue:** The Lv5 HP damage block is OUTSIDE the `if (targetCast)` check. In rAthena, ALL Spell Breaker effects (including Lv5 damage) require target to be casting.
- **Fix:** Move the `if (learnedLevel >= 5)` block inside `if (targetCast)`.

---

### 8. Dragonology (1407) — Passive

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| INT bonus | [1,1,**2**,2,3] = `floor((lv+1)/2)` | [1,1,**1**,2,3] | BUG #13 |
| Dragon Phys ATK | +4%/lv (4,8,12,16,20) | `raceATK.dragon = lv*4` | CORRECT |
| Dragon Magic ATK | +2%/lv (2,4,6,8,10) | NOT IMPLEMENTED | BUG #14 |
| Dragon Resist | +4%/lv (4,8,12,16,20) | `raceResist.dragon` stored | CORRECT (stored) |
| Resist in damage calc | Reduce incoming Dragon damage | NEVER APPLIED (dead code) | BUG #15 |

**Source:** divine-pride Skill 284 (shows separate phys/magic columns), iRO Wiki Dragonology, RateMyServer. INT formula: `floor((skillLevel + 1) / 2)`. Magical ATK bonus is HALF of physical (+2% vs +4%).

**BUG #13 — Dragonology INT bonus wrong at Lv3**
- **Severity:** MODERATE
- **Location:** `index.js` line 580
- **Issue:** Array `[1, 1, 1, 2, 3]` — Lv3 should be +2, not +1. Canonical: `[1, 1, 2, 2, 3]`.
- **Fix:** Change to `[1, 1, 2, 2, 3]`.

**BUG #14 — Dragonology magical ATK bonus vs Dragons missing**
- **Severity:** MODERATE
- **Location:** `getPassiveSkillBonuses()` + `calculateMagicalDamage()`
- **Issue:** Only physical Dragon ATK bonus (raceATK.dragon) implemented. Pre-renewal has a SEPARATE +2%/lv magical ATK bonus vs Dragon race.
- **Fix:** Add `raceMATK.dragon = dragLv * 2` in passives, apply in `calculateMagicalDamage()`.

**BUG #15 — Dragonology Dragon resist dead code**
- **Severity:** CRITICAL
- **Location:** `index.js` line 2682 (missing from buildFullStatsPayload)
- **Issue:** `raceResist.dragon` stored but never in stats payload or damage pipeline.
- **Fix:** Add `passiveRaceResist` to payload, apply as `% damage reduction` in both physical and magical damage functions.

---

### 9. Endow Blaze (1408) / Tsunami (1409) / Tornado (1410) / Quake (1411)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | 40 all levels | 40 | CORRECT |
| Cast time | 3000ms | 3000ms | CORRECT |
| Success rate | [70,80,90,100,100]% | Exact match | CORRECT |
| Duration | Lv1-4: 20min, Lv5: 30min | Bimodal correct | CORRECT |
| Catalysts | Red Blood/Crystal Blue/Wind of Verdure/Green Live | In SKILL_CATALYSTS | CORRECT |
| Mutual exclusion | Priority order (multiple can coexist, highest priority wins) | Removes old endows before applying | MINOR |
| Innate element check | No check in rAthena (endow overwrites innate) | No check | CORRECT |
| Buff expiry revert | Revert weaponElement | NOT IMPLEMENTED | BUG #16 |
| Target | Self + Party + Guild | targetId or self | CORRECT |

**Source:** rAthena `skill_require_db.txt` (catalysts), `status_calc_attack_element()` (priority order), iRO Wiki Classic. rAthena source shows NO check for innate weapon element — endow simply overwrites via status. Multiple endows can coexist with priority: Water > Earth > Fire > Wind > Enchant Poison > Aspersio.

**Correction from original audit:** BUG #7 (endow innate element check) was WRONG — rAthena does NOT block endow on elemental weapons. Endow just takes precedence. Removed as a bug.

**BUG #16 — Endow weaponElement not reverted on buff expiry**
- **Severity:** CRITICAL
- **Issue:** When endow buff expires, `weaponElement` stays as endowed element permanently.
- **Fix:** In buff expiry tick, check for `endow_*` prefix and revert to `baseWeaponElement || 'neutral'`.

---

### 10. Volcano (1412)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | [48,46,44,42,40] | `48-i*2` | CORRECT |
| Cast time | 5000ms | 5000ms | CORRECT |
| Range | 2 cells (100 UE) | 100 | CORRECT |
| AoE | 7x7 (175 UE radius) | `radius: 175` | CORRECT |
| Duration | [60,120,180,240,300]s | `lv * 60000` | CORRECT |
| Fire dmg boost | [10,14,17,19,20]% | Stored but NEVER applied | BUG #17 |
| ATK bonus | **[10,20,30,40,50]** flat | **[10,15,20,25,30]** | BUG #18 |
| ATK only for fire-element entities | Pre-renewal: `def_ele == ELE_FIRE` check | Applies to ALL | BUG #19 |
| Catalyst | Yellow Gemstone (715) | In SKILL_CATALYSTS | CORRECT |
| Sage zone mutex | Only 1 of Vol/Del/VG | Removes others | CORRECT |

**Source:** rAthena `status.cpp`: `val2 = val1*10` (ATK = skillLevel * 10), `enchant_eff[]` = {10,14,17,19,20} (fire dmg%), `#ifndef RENEWAL` block shows `if (status->def_ele != ELE_FIRE) val2 = 0`.

### 11. Deluge (1413)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Water dmg boost | [10,14,17,19,20]% | Stored but NEVER applied | BUG #17 |
| MaxHP bonus | [5,9,12,14,15]% | Treated as FLAT, not % | BUG #20 |
| MaxHP only for water-element | Pre-renewal: `def_ele == ELE_WATER` check | Applies to ALL | BUG #19 |
| Creates water terrain | Yes (for Water Ball/Aqua Benedicta) | NOT IMPLEMENTED | DEFERRED |

### 12. Violent Gale (1414)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Wind dmg boost | [10,14,17,19,20]% | Stored but NEVER applied | BUG #17 |
| FLEE bonus | [3,6,9,12,15] flat | Applied via buff | CORRECT |
| FLEE only for wind-element | Pre-renewal: `def_ele == ELE_WIND` check | Applies to ALL | BUG #19 |
| Fire Wall duration +150% | Pre-renewal bonus | NOT IMPLEMENTED | DEFERRED |

**BUG #17 — Zone elemental damage boosts not applied in damage pipeline** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** `ro_damage_formulas.js` (missing entirely)
- **Issue:** `fireDmgBoost`/`waterDmgBoost`/`windDmgBoost` stored in buff modifiers but NEVER read by any damage function. Zero matches in `ro_damage_formulas.js`.
- **Fix:** After element modifier in both `calculatePhysicalDamage()` and `calculateMagicalDamage()`, apply:
  ```js
  if (element === 'fire' && buffMods.fireDmgBoost) damage *= (100 + fireDmgBoost) / 100;
  // Same for water/wind
  ```

**BUG #18 — Volcano ATK bonus values wrong** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** `index.js` line 11605
- **Issue:** Current: `[10, 15, 20, 25, 30]`. rAthena: `val2 = val1 * 10` = `[10, 20, 30, 40, 50]`.
- **Fix:** Change to `[10, 20, 30, 40, 50]`.

**BUG #19 — Zone stat bonuses apply to all entities (should be element-restricted in pre-renewal)** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** Ground effect tick loop, `index.js` lines 19908-19940
- **Issue:** In pre-renewal, Volcano ATK only applies to fire-element entities, Deluge MaxHP only to water-element, Violent Gale FLEE only to wind-element. Our code applies to ALL entities.
- **Fix:** In the ground effect tick, check `player.armorElement || player.defElement || 'neutral'` before applying stat bonuses. Only apply if matching element.

**BUG #20 — Deluge MaxHP bonus treated as flat instead of percentage** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** `ro_buff_system.js` line 920
- **Issue:** `mods.bonusMaxHp += buff.maxHpBoost` — treated as flat HP. Should be percentage.
- **Fix:** Change to `mods.bonusMaxHpPercent = (mods.bonusMaxHpPercent || 0) + (buff.maxHpBoost || 0)` (same pattern as Apple of Idun on line 845).

---

### 13. Land Protector (1415)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | [66,62,58,54,50] | `[66,62,58,54,50]` | CORRECT |
| Cast time | 5000ms | 5000ms | CORRECT |
| Range | **2 cells (100 UE)** | **150 (3 cells)** | BUG #21 |
| AoE by level | Lv1-2: 7x7, Lv3-4: 9x9, Lv5: 11x11 | `[175,175,225,225,275]` | CORRECT |
| Duration | [120,165,210,255,300]s | `120000+i*45000` | CORRECT |
| Catalyst | Blue Gem + Yellow Gem | In SKILL_CATALYSTS | CORRECT |
| Removes existing effects | On creation | Iterates and removes | CORRECT |
| Blocks new effects | While active | `createGroundEffect()` LP check | CORRECT |
| LP vs LP | Mutual destruction (both removed) | NOT IMPLEMENTED | BUG #22 |
| Does NOT block traps | Traps have INF2_ISTRAP | Traps not in LP_BLOCKED | CORRECT |
| LP_BLOCKED list | Consistent across codebase | INCONSISTENT (2 lists) | BUG #23 |

**Source:** rAthena `skill_db.yml` (range: 2), `skill.cpp` line 11214 (LP handler), iRO Wiki Classic Magnetic Earth, Divine Pride.

**BUG #21 — Land Protector range wrong**
- **Severity:** MODERATE
- **Location:** `ro_skill_data_2nd.js` line 74
- **Issue:** `range: 150` (3 cells). rAthena confirms range is 2 cells = 100 UE.
- **Fix:** Change to `range: 100`.

**BUG #22 — LP vs LP mutual destruction not implemented**
- **Severity:** MODERATE
- **Location:** `index.js` LP handler lines 11714-11765
- **Issue:** When LP is placed on top of another LP (from a different caster), both should be destroyed. rAthena: `if (unit->group->skill_id == SA_LANDPROTECTOR) { (*alive) = 0; skill_delunit(unit); return 1; }`.
- **Fix:** In LP creation, check for existing LP in area. If found from different caster, remove both.

**BUG #23 — LP_BLOCKED list inconsistency**
- **Severity:** LOW
- **Location:** `index.js` lines 11738-11742 vs `ro_ground_effects.js` lines 41-48
- **Fix:** Consolidate to one authoritative list. Remove `thunderstorm` and `frost_nova` (not persistent ground effects).

---

### 14. Abracadabra (1416)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| SP cost | 50 | 50 | CORRECT |
| Cooldown | **0ms** | **3000ms** | BUG #24 |
| Catalyst | 2 Yellow Gems | In SKILL_CATALYSTS | CORRECT |
| Random skill cast | Full pool of 100+ skills + exclusive effects | STUB | DEFERRED |

**Source:** rAthena pservero pre-re DB (cooldown 0.00s, ACD 0.00s).

**BUG #24 — Abracadabra cooldown should be 0**
- **Severity:** LOW (skill is a stub anyway)
- **Location:** `ro_skill_data_2nd.js` line 75
- **Fix:** Change `cooldown: 3000` to `cooldown: 0`.

---

### 15. Earth Spike Sage (1417)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Damage per hit | **100% MATK** (pre-renewal) | **200% MATK** (Renewal value) | BUG #25 |
| Hits | = skill level (1-5) | `effectValue: i+1` | CORRECT |
| SP cost | [12,14,16,18,20] | `12+i*2` | CORRECT |
| Cast time | [700,1400,2100,2800,3500]ms | `700*(i+1)` | CORRECT |
| After-cast delay | [1000,1200,1400,1600,1800]ms | `afterCastDelay: 0` | BUG #26 |
| Element | Earth | Earth | CORRECT |
| Can hit hidden | YES | Handled by handler | CORRECT |

**Source:** iRO Classic Wiki Earth Spike ("100% MATK per hit"), rAthena commit c3ff388 (ACD corrected to per-level). The 200% MATK figure is RENEWAL ONLY.

**BUG #25 — Earth Spike damage uses Renewal 200% instead of pre-renewal 100%** (CRITICAL)
- **Severity:** CRITICAL
- **Location:** Handler in `index.js` (shared with Wizard Earth Spike 804)
- **Issue:** The handler uses 200% MATK per hit (Renewal). Pre-renewal is 100% MATK per hit.
- **Fix:** Change MATK multiplier to 100 in the handler. Also fix the Hindsight autocast which uses `matkPctPerLv: 200`.

**BUG #26 — Earth Spike Sage missing after-cast delay**
- **Severity:** MODERATE
- **Location:** `ro_skill_data_2nd.js` line 76
- **Fix:** Change to `afterCastDelay: 1000+i*200` giving [1000,1200,1400,1600,1800]ms.

---

### 16. Heaven's Drive Sage (1418)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| Damage per hit | **100% MATK** (pre-renewal) | **125% MATK** (incorrect value) | BUG #25 |
| After-cast delay | **1000ms flat** | `afterCastDelay: 0` | BUG #27 |
| SP cost | [28,32,36,40,44] | `28+i*4` | CORRECT |
| AoE | 5x5 | 125 UE radius | CORRECT |

**Source:** iRO Wiki Classic Heaven's Drive ("basically a 5x5 AoE version of Earth Spike" — same 100% MATK). rAthena commit c3ff388 (ACD = 1000ms for pre-re).

**BUG #25 (continued) — Heaven's Drive uses 125% instead of pre-renewal 100%**
- **Fix:** Change MATK multiplier to 100 in shared handler.

**BUG #27 — Heaven's Drive Sage ACD is 1000ms, not 500ms**
- **Severity:** MODERATE
- **Location:** `ro_skill_data_2nd.js` line 77
- **Fix:** Change to `afterCastDelay: 1000` (flat all levels). Original audit said 500ms — corrected by rAthena commit c3ff388.

---

### 17. Sense Sage (1419)

**Status: FULLY CORRECT** — SP 10, instant, reveals HP/element/size/race/DEF/MDEF/level. No changes needed.

### 18-19. Create Elemental Converter (1420) / Elemental Change (1421)

**Status: STUBS** — Intentionally deferred.

**Note for Elemental Change:** ACD should be 1000ms (rAthena confirms), currently 0.

---

## Complete Bug List (Priority-Ordered)

### Phase 1: Critical Damage Pipeline (6 bugs)

| # | Bug | Description | Effort |
|---|-----|-------------|--------|
| 1 | #25 | Earth Spike/Heaven's Drive: 200%/125% MATK → 100% MATK (pre-renewal) | Change multiplier in shared handler + Hindsight autocast |
| 2 | #17 | Zone damage boosts (fire/water/wind) dead code in damage pipeline | Add elemental boost checks to calculatePhysicalDamage + calculateMagicalDamage |
| 3 | #18 | Volcano ATK: [10,15,20,25,30] → [10,20,30,40,50] | Fix array in handler |
| 4 | #19 | Zone stat bonuses apply to all (should be element-restricted pre-renewal) | Add element check in ground effect tick |
| 5 | #20 | Deluge MaxHP flat → percentage | Change bonusMaxHp to bonusMaxHpPercent |
| 6 | #15 | Dragonology resist dead code | Add passiveRaceResist to payload + damage pipeline |

### Phase 2: Critical Functional Gaps (4 bugs)

| # | Bug | Description | Effort |
|---|-----|-------------|--------|
| 7 | #4 | Hindsight spell pool uses RENEWAL groups (Napalm Beat missing, ES/TS/HD shouldn't be in pool) | Rewrite autocast spell pool to pre-renewal per-level table |
| 8 | #8 | Magic Rod absorption never checked in damage paths | Add magic_rod check to all single-target magic handlers |
| 9 | #16 | Endow weaponElement not reverted on buff expiry | Add endow check in buff expiry tick |
| 10 | #9 | Magic Rod vs Spell Breaker counter missing | Add MR check in SB handler |

### Phase 3: Skill Data Fixes (7 bugs)

| # | Bug | Description | Effort |
|---|-----|-------------|--------|
| 11 | #2 | Hindsight duration data: 60000+i*30000 → 120000+i*30000 | One-line fix |
| 12 | #26 | Earth Spike Sage ACD: 0 → 1000+i*200 | One-line fix |
| 13 | #27 | Heaven's Drive Sage ACD: 0 → 1000 | One-line fix |
| 14 | #5 | Dispell cooldown: 2000 → 0 | One-line fix |
| 15 | #24 | Abracadabra cooldown: 3000 → 0 | One-line fix |
| 16 | #7 | Magic Rod duration data: 200+i*200 → 400+i*200 | One-line fix |
| 17 | #13 | Dragonology INT: [1,1,1,2,3] → [1,1,2,2,3] | One-line fix |

### Phase 4: Moderate Issues (5 bugs)

| # | Bug | Description | Effort |
|---|-----|-------------|--------|
| 18 | #1 | Advanced Book ASPD bonus missing (+0.5%/lv) | Add to passives + calculateASPD |
| 19 | #14 | Dragonology magic ATK bonus missing (+2%/lv vs dragons) | Add raceMATK.dragon to passives + magic damage |
| 20 | #12 | Spell Breaker Lv5 damage fires without target casting | Move inside if(targetCast) |
| 21 | #10 | Free Cast speed reduction not enforced | Server-side speed validation |
| 22 | #21 | Land Protector range: 150 → 100 | One-line fix |

### Phase 5: Low Priority (3 bugs)

| # | Bug | Description | Effort |
|---|-----|-------------|--------|
| 23 | #6 | Dispell UNDISPELLABLE set incomplete | Add entries as systems are built |
| 24 | #22 | LP vs LP mutual destruction | Add check in LP creation |
| 25 | #23 | LP_BLOCKED list inconsistency | Consolidate to one list |

---

## Corrections From Original Audit (v1 → v2)

| Item | v1 (Wrong) | v2 (Correct) | Source |
|------|-----------|-------------|--------|
| Hindsight duration canonical | 90s-360s | 120s-390s | rAthena `skill_cast_db.txt` |
| Hindsight handler | WRONG | CORRECT (120s-390s) | rAthena confirms |
| Heaven's Drive ACD | 500ms | 1000ms | rAthena commit c3ff388 |
| Endow innate element check | Missing (was BUG) | Not required (rAthena has no check) | rAthena source `skill.cpp` |
| Earth Spike MATK% | 200% (assumed correct) | 100% pre-renewal | iRO Classic Wiki |
| Heaven's Drive MATK% | 125% (assumed correct) | 100% pre-renewal | iRO Classic Wiki |
| Volcano ATK | [10,15,20,25,30] (assumed correct) | [10,20,30,40,50] | rAthena `status.cpp` |
| Zone bonuses | Apply to all (assumed correct) | Element-restricted in pre-renewal | rAthena `#ifndef RENEWAL` |
| Dragonology INT | [1,1,1,2,3] (assumed correct) | [1,1,2,2,3] | divine-pride `floor((lv+1)/2)` |
| Hindsight spell pool | Current impl. assumed roughly correct | Uses RENEWAL groups, pre-renewal is per-level | iRO Classic Wiki |

---

## New Findings (Not in Original Audit)

1. **Hindsight spell pool is RENEWAL, not pre-renewal** — Napalm Beat missing, Earth Spike/Thunderstorm/Heaven's Drive shouldn't be in pool
2. **Earth Spike/Heaven's Drive damage is 100% MATK in pre-renewal** — our 200%/125% values are Renewal
3. **Volcano ATK is [10,20,30,40,50]** — not [10,15,20,25,30]
4. **Zone stat bonuses are element-restricted in pre-renewal** — only fire/water/wind-element entities get ATK/MaxHP/FLEE
5. **Dragonology INT is [1,1,2,2,3]** — Lv3 should be +2
6. **Dragonology has separate +2%/lv magical ATK** — half of physical bonus
7. **Spell Breaker Lv5 damage should require target to be casting** — currently fires regardless
8. **Magic Rod vs Spell Breaker counter** — SB caster loses 20% MaxSP
9. **LP vs LP mutual destruction** — both destroyed when overlapping
10. **Dispell cooldown should be 0** — currently 2000ms
11. **Abracadabra cooldown should be 0** — currently 3000ms
12. **Land Protector range is 2 cells (100 UE)** — currently 150

---

## Verification Checklist

After all fixes, verify each skill:

- [ ] **Hindsight Lv1:** Autocast Napalm Beat (not bolts). Duration 120s.
- [ ] **Hindsight Lv10:** Pool includes Napalm Beat, 3 Bolts, Soul Strike, Fire Ball, Frost Diver. NO Earth Spike/Thunderstorm/Heaven's Drive.
- [ ] **Earth Spike Lv5:** 5 hits at 100% MATK each = 500% total (not 1000%).
- [ ] **Heaven's Drive Lv5:** 5 hits at 100% MATK each, AoE 5x5, ACD 1000ms.
- [ ] **Volcano Lv5:** +50 ATK (not +30), only for fire-element entities. +20% fire damage boost applied in combat.
- [ ] **Deluge Lv5:** +15% MaxHP (percentage, not flat +15), only for water-element entities.
- [ ] **Violent Gale Lv5:** +15 FLEE, only for wind-element entities. +20% wind damage boost in combat.
- [ ] **Dragonology Lv3:** +2 INT (not +1). Attack Dragon: physical +12%, magical +6%. Take hit from Dragon: -12% damage.
- [ ] **Magic Rod:** Cast, receive bolt, verify damage nullified + SP restored. Movement cancels window.
- [ ] **Spell Breaker:** Fails if target not casting. Lv5 damage only when target IS casting. Against Magic Rod user: SB fails, lose 20% MaxSP.
- [ ] **Dispell:** No cooldown. Hindsight survives. Endow reverts weaponElement.
- [ ] **Endow:** Buff expires → weaponElement reverts. No innate element check needed.
- [ ] **Land Protector:** Range 2 cells. LP on LP = both destroyed.
