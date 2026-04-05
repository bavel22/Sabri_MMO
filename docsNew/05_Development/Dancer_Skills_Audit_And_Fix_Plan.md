# Dancer Skills Comprehensive Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Dancer_Class_Research](Dancer_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-17
**Status:** IMPLEMENTED — ALL FIXES APPLIED
**Scope:** All 20 Dancer skills (IDs 1520-1557) — 12 solo + 8 ensemble
**Primary Source:** eathena `src/map/skill.c` + `src/map/status.c` (pre-renewal C source code)
**Secondary Sources:** rAthena pre-renewal `skill_db.yml`, rAthena GitHub issues #1808/#7747/#6916, iRO Wiki, rAthena split commit #9771/#9401

---

## Source Code Evidence

All formulas below are verified against the **actual C source code** from eathena/rAthena. Exact code snippets are included for each bug.

### eathena `skill.c` — Performance Stat Values (status_change_start)

```c
case DC_HUMMING:
    val1 = 2*skilllv+status->dex/10; // Hit increase
    if(sd) val1 += pc_checkskill(sd,DC_DANCINGLESSON);
    break;

case DC_DONTFORGETME:
    val1 = status->dex/10 + 3*skilllv + 5; // ASPD decrease
    val2 = status->agi/10 + 3*skilllv + 5; // Movement speed adjustment.
    if(sd){ val1 += pc_checkskill(sd,DC_DANCINGLESSON);
            val2 += pc_checkskill(sd,DC_DANCINGLESSON); }
    break;

case DC_FORTUNEKISS:
    val1 = 10+skilllv+(status->luk/10); // Critical increase
    if(sd) val1 += pc_checkskill(sd,DC_DANCINGLESSON);
    val1*=10; //Because every 10 crit is an actual cri point.
    break;

case DC_SERVICEFORYOU:
    val1 = 15+skilllv+(status->int_/10); // MaxSP percent increase
    val2 = 20+3*skilllv+(status->int_/10); // SP cost reduction
    if(sd){ val1 += pc_checkskill(sd,DC_DANCINGLESSON);  //TO-DO Should be half
            val2 += pc_checkskill(sd,DC_DANCINGLESSON); } //TO-DO Should be half
    break;
```

### eathena `skill.c` — Ugly Dance SP Drain (skill_additional_effect)

```c
case DC_UGLYDANCE:
    rate = 5+5*skilllv;
    if(sd && (skill=pc_checkskill(sd,DC_DANCINGLESSON)))
        rate += 5+skill;
    status_zap(bl, 0, rate);
    break;
```

### rAthena `winkofcharm.cpp` — Charming Wink (pre-renewal)

```cpp
// Against Players (pre-renewal):
if (sc_start(src, target, SC_CONFUSION, 10, skill_lv, Duration1)) // 10% chance, 30s
    sc_start(src, target, SC_WINKCHARM, 100, skill_lv, Duration2);  // then charm 10s

// Against Monsters:
sc_start2(src, target, SC_WINKCHARM,
    (status_get_lv(src) - status_get_lv(target)) + 40, // Level-based chance
    skill_lv, src->id, Duration2); // 10s charm (follow caster)
```

### rAthena `dazzler.cpp` — Scream (pre-renewal)

```cpp
int32 rate = 150 + 50 * skill_lv + 100; // Aegis: 1000=100%. Lv1=300=30%
if (battle_check_target(src, target, BCT_PARTY) > 0)
    rate /= 4; // Party members: 1/4 chance
skill_addtimerskill(src, tick+3000, ...); // 3-second delayed execution
```

### rAthena `slingingarrow.cpp` — Slinging Arrow (pre-renewal)

```cpp
base_skillratio += -40 + 40 * skill_lv; // Total: 60+40*Lv (100%/140%/180%/220%/260%)
```

---

## Audit Summary

| Category | Count |
|----------|-------|
| **Critical Bugs** (broken/wrong functionality) | 8 |
| **Moderate Bugs** (incorrect values) | 5 |
| **Minor/Deferred** (design deviations) | 8 |

---

## CRITICAL BUGS (8)

### BUG-1: bonusMaxSpPercent NEVER consumed — Service For You MaxSP% has NO EFFECT

**Severity:** CRITICAL — feature is completely non-functional
**Location:** `server/src/index.js` line ~2674 (`getEffectiveStats`)
**Also affects:** Apple of Idun (Bard) `bonusMaxHpPercent` — same bug

**Problem:** `dance_service_for_you` buff writes `maxSpPercent` into the buff. `getBuffModifiers()` stores it as `mods.bonusMaxSpPercent`. But `getEffectiveStats` builds `bonusMaxSpRate` ONLY from cards+equip — it never includes the buff value:

```js
// CURRENT (broken):
bonusMaxSpRate: (player.cardMaxSpRate || 0) + (player.equipMaxSpRate || 0),

// FIX:
bonusMaxSpRate: (player.cardMaxSpRate || 0) + (player.equipMaxSpRate || 0) + (buffMods.bonusMaxSpPercent || 0),
```

**ALSO FIX Apple of Idun (Bard):**
```js
// CURRENT (broken):
bonusMaxHpRate: (player.cardMaxHpRate || 0) + (player.equipMaxHpRate || 0),

// FIX:
bonusMaxHpRate: (player.cardMaxHpRate || 0) + (player.equipMaxHpRate || 0) + (buffMods.bonusMaxHpPercent || 0),
```

---

### BUG-2: PDFM ASPD reduction formula missing +5 base constant

**Severity:** CRITICAL
**Source:** `val1 = status->dex/10 + 3*skilllv + 5;` (eathena skill.c)

**Current:** `aspdReduction: skillLevel * 3 + Math.floor(effectiveStats.dex / 10) + lessonsLv`
**Fix:** `aspdReduction: 5 + skillLevel * 3 + Math.floor(effectiveStats.dex / 10) + lessonsLv`

**Impact:** Lv10/DL10/DEX99: 49% → 54% (off by 5%)

---

### BUG-3: PDFM Move Speed reduction formula completely wrong (3 errors)

**Severity:** CRITICAL
**Source:** `val2 = status->agi/10 + 3*skilllv + 5; val2 += DL;` (eathena skill.c)

**Current:** `moveSpeedReduction: skillLevel * 2 + Math.floor(effectiveStats.agi / 10) + Math.ceil(lessonsLv / 2)`

**Three errors:**
1. Missing +5 base constant
2. Multiplier is `2*lv` instead of `3*lv`
3. DL contribution is `ceil(DL/2)` instead of full `DL`

**Fix:** `moveSpeedReduction: 5 + skillLevel * 3 + Math.floor(effectiveStats.agi / 10) + lessonsLv`

**Impact:** Lv10/DL10/AGI99: 34% → 54% (off by 20 percentage points!)

---

### BUG-4: Charming Wink — completely wrong mechanics (PvE and PvP)

**Severity:** CRITICAL — wrong effect, wrong success rate, wrong duration
**Source:** rAthena `winkofcharm.cpp` pre-renewal block

**Current:** Flat 70% confusion on monsters for 10s.

**Canonical pre-renewal:**

| Target | Success Rate | Effect | Duration |
|--------|-------------|--------|----------|
| Monster | `(casterBaseLv - targetBaseLv) + 40`% | SC_WINKCHARM: monster follows caster (charmed, non-aggressive) | 10s |
| Player | 10% confusion | SC_CONFUSION (30s), then SC_WINKCHARM (10s) only if confusion lands | 30s/10s |

**Errors in current implementation:**
1. PvE success rate is flat 70% — should be level-based formula
2. PvE effect is confusion — should be charm (deaggro + follow caster)
3. PvP is completely missing (10% confusion gate → then charm)

**Fix:** Rewrite handler:
- Monster: `chance = Math.max(0, (player.baseLv - enemy.level) + 40)`, on success set `enemy.charmedUntil = Date.now() + 10000`, `enemy.charmedBy = characterId`, clear aggro
- In enemy AI tick: if `charmedUntil > Date.now()`, move toward `charmedBy` player position, skip aggro

---

### BUG-5: Ugly Dance SP drain missing Dance Lessons bonus

**Severity:** CRITICAL
**Source:** `rate = 5+5*skilllv; if(DL>0) rate += 5+DL;` (eathena skill.c)

**Current:** `spDrainPerTick: 5 + 5 * skillLevel`
**Fix:** `spDrainPerTick: (5 + 5 * skillLevel) + (lessonsLv > 0 ? 5 + lessonsLv : 0)`

**Impact:** Lv5/DL10: 30 → 45 SP/tick (50% increase)

---

### BUG-6: Fortune's Kiss CRIT formula missing +10 base constant

**Severity:** CRITICAL — CRIT bonus is 10 lower than intended
**Source:** `val1 = 10+skilllv+(status->luk/10); val1 += DL; val1*=10;` (eathena skill.c)

**Current:** `critBonus: skillLevel + Math.floor(effectiveStats.luk / 10) + lessonsLv`

The eathena code has a +10 base before the skill level. The `*10` at the end converts to internal 0.1-crit units. Since our system stores CRIT as display integers (not *10), our formula should be:

**Fix:** `critBonus: 10 + skillLevel + Math.floor(effectiveStats.luk / 10) + lessonsLv`

**Impact:** Lv10/LUK99/DL10: 29 → 39 CRIT (off by 10!)

---

### BUG-7: Service For You MaxSP% formula missing Dance Lessons term

**Severity:** CRITICAL
**Source:** `val1 = 15+skilllv+(status->int_/10); val1 += DL;` (eathena skill.c, with TODO "should be half")

**Current:** `maxSpPercent: 15 + skillLevel + Math.floor(effectiveStats.int / 10)`

The eathena code adds full DL (with a TODO noting it should be DL/2). We'll use full DL to match the actual pre-renewal code, not the aspirational TODO.

**Fix:** `maxSpPercent: 15 + skillLevel + Math.floor(effectiveStats.int / 10) + lessonsLv`

**Impact:** Lv10/INT99/DL10: 34% → 44% MaxSP

**Note:** BUG-1 must also be fixed or this value still has no effect.

---

### BUG-8: Service For You SP reduction DL contribution should match MaxSP

**Severity:** MODERATE-HIGH
**Source:** `val2 = 20+3*skilllv+(status->int_/10); val2 += DL;` (eathena skill.c, TODO "should be half")

**Current:** `spCostReduction: 20 + skillLevel * 3 + Math.floor(effectiveStats.int / 10) + Math.ceil(lessonsLv / 2)`

The eathena code uses full DL (not DL/2). Our code uses `ceil(DL/2)`. To match the actual source:

**Fix:** `spCostReduction: 20 + skillLevel * 3 + Math.floor(effectiveStats.int / 10) + lessonsLv`

**Impact:** Lv10/INT99/DL10: 64% → 69% SP reduction

---

## MODERATE BUGS (5)

### BUG-9: Scream has 3-second delayed execution — currently instant

**Severity:** MODERATE — affects gameplay timing significantly
**Source:** `skill_addtimerskill(src, tick+3000, ...)` (rAthena dazzler.cpp)

**Current:** Stun check fires immediately on cast.
**Canonical:** Stun check fires 3 seconds AFTER casting. This gives targets time to move out of range. Same behavior as Bard's Frost Joker.

**Fix:** In Scream handler, instead of applying stun immediately, set a 3-second `setTimeout` that then checks all entities in screen range and applies stun. Requires tracking the caster's position at cast time.

---

### BUG-10: Scream party member stun chance should be 1/4

**Severity:** MODERATE
**Source:** `if (battle_check_target(src, target, BCT_PARTY) > 0) rate /= 4;` (rAthena dazzler.cpp)

**Current:** All entities get full stun chance.
**Canonical:** Party members receive only 25% of the stun chance.

**Fix:** When iterating players, check if target is in same party as caster. If so, divide stun chance by 4.
**Note:** Party system is deferred, so this fix should be stubbed (always full rate) and activated when parties are implemented.

---

### BUG-11: Charming Wink ACD should be 2000ms, not 1000ms

**Severity:** MODERATE
**Source:** rAthena pre-re skill_cast_db: `AfterCastActDelay: 2000`

**Current:** `afterCastDelay: 1000`
**Fix:** `afterCastDelay: 2000`

---

### BUG-12: Charming Wink cooldown should be 0, not 3000ms

**Severity:** MODERATE
**Source:** rAthena pre-re skill_cast_db: `Cooldown: 0`

**Current:** `cooldown: 3000`
**Fix:** `cooldown: 0`

---

### BUG-13: Moonlit Water Mill duration formula wrong

**Severity:** MODERATE
**Source:** rAthena skill_db.yml Duration1: 20000:25000:30000:35000:40000

**Current:** `(24 + i * 4) * 1000` → 24s, 28s, 32s, 36s, 40s
**Canonical:** 20s, 25s, 30s, 35s, 40s (formula: `15 + 5*Lv`)
**Fix:** `(15 + (i + 1) * 5) * 1000`

---

## MINOR/DEFERRED ISSUES (8)

### DEF-1: PDFM should remove speed/ASPD buffs when applied

**Status:** DEFERRED until buff cancellation system is more complete
**Source:** eathena status.c — `case SC_DONTFORGETME:` removes SC_INCREASEAGI, SC_ADRENALINE, SC_ADRENALINE2, SC_SPEARQUICKEN, SC_TWOHANDQUICKEN, SC_ONEHAND

### DEF-2: Moonlit Water Mill is canonically an ensemble, not solo dance

**Status:** KNOWN DESIGN DECISION — ensemble system deferred until party system

### DEF-3: Moonlit Water Mill attack-blocking/entry-blocking not implemented

**Status:** DEFERRED — requires collision/combat tick integration

### DEF-4: Moonlit Water Mill knockback 2 on cast not implemented

**Status:** DEFERRED

### DEF-5: Scream allowed during performance (matches rAthena behavior)

**Status:** CORRECT — rAthena's Dazzler handler is called via `skill_addtimerskill` which works regardless of performance state. Our implementation correctly allows it.

### DEF-6: Adaptation "last 5 seconds" restriction is non-canonical (only first 5s in rAthena)

**Status:** MINOR DEVIATION — keep current conservative behavior

### DEF-7: Slinging Arrow damage formula confirmed correct

**Status:** VERIFIED — rAthena `slingingarrow.cpp`: `base_skillratio += -40 + 40*skill_lv` = total `60+40*Lv` = 100/140/180/220/260%. Matches our implementation exactly.

### DEF-8: Dance Lessons movement during performance not implemented

**Status:** DEFERRED — requires performance movement system

---

## VERIFICATION: Skills Confirmed Correct

### Dance Lessons (1520) — CORRECT
- [x] +3 ATK/level with whips (eathena battle.cpp: `damage += skill * 3` for W_WHIP)
- [x] No Renewal-only bonuses (MaxSP%, CRIT behind `#ifdef RENEWAL`)
- [x] DL level used in all dance formulas in `calculatePerformanceEffects`

### Ugly Dance (1525) — SP costs/duration CORRECT, drain formula BUGGED (BUG-5)
- [x] SP Cost: 23/26/29/32/35 matches `20+3*lv`
- [x] Duration: 30000ms
- [x] effectValue: 10/15/20/25/30 matches `5+5*lv` base drain
- [ ] **BUGGED**: Missing `+ (5 + DL)` bonus (BUG-5)

### Humming (1522) — CORRECT (matches eathena source)
- [x] SP Cost: 22/24/.../40 matches `20+2*lv`
- [x] Duration: 60000ms, Linger: 20000ms
- [x] HIT formula: `2*lv + DEX/10 + DL` — eathena code confirms NO +1 constant
- [x] Note: rAthena issue #1808 (official server testing) found +1, but eathena code does not include it. Our implementation matches the code.

### Please Don't Forget Me (1527) — SP costs CORRECT, BOTH formulas BUGGED (BUG-2, BUG-3)
- [x] SP Cost: 28/31/.../55 matches `25+3*lv`
- [x] Duration: 180000ms
- [ ] **BUGGED**: ASPD missing +5 base, formula = `5 + 3*lv + DEX/10 + DL` (BUG-2)
- [ ] **BUGGED**: Move missing +5, wrong multiplier (2→3), wrong DL (ceil(DL/2)→DL) (BUG-3)

### Fortune's Kiss (1528) — SP costs CORRECT, CRIT formula BUGGED (BUG-6)
- [x] SP Cost: 43/46/.../70 matches `40+3*lv`
- [x] Duration: 120000ms
- [ ] **BUGGED**: Missing +10 base constant. Source: `val1 = 10+skilllv+(luk/10)+DL` then `*10` (BUG-6)

### Service For You (1521) — SP costs CORRECT, MaxSP BUGGED (BUG-1, BUG-7, BUG-8)
- [x] SP Cost: 40/45/.../85 matches `35+5*lv`
- [x] Duration: 180000ms
- [ ] **BUGGED**: `bonusMaxSpPercent` never consumed (BUG-1)
- [ ] **BUGGED**: MaxSP missing DL — source uses full DL (BUG-7)
- [ ] **BUGGED**: SP reduction uses ceil(DL/2) — source uses full DL (BUG-8)

### Slinging Arrow (1541) — VERIFIED CORRECT
- [x] SP Cost: 1/3/5/7/9 matches `2*lv-1`
- [x] Cast Time: 1500ms
- [x] Damage: 100/140/180/220/260% — rAthena confirms `60+40*lv` (not RMS values)
- [x] Whip required, arrow consumed, usable during performance

### Scream (1526) — Stun chance CORRECT, execution timing BUGGED (BUG-9, BUG-10)
- [x] SP Cost: 12/14/16/18/20 matches `10+2*lv`
- [x] Stun chance: 30/35/40/45/50% matches `(250+50*lv)/10`
- [x] Screen-wide, affects all (friend/foe), boss immune
- [ ] **BUGGED**: Executes instantly — should have 3s delay (BUG-9)
- [ ] **BUGGED**: Party members should get 1/4 stun chance (BUG-10, deferred)

### Charming Wink (1529) — EXTENSIVELY BUGGED (BUG-4, BUG-11, BUG-12)
- [x] SP Cost: 40
- [x] Cast Time: 1000ms
- [x] Race restriction: Demi-Human, Angel, Demon
- [x] Boss immune
- [ ] **BUGGED**: Wrong PvE effect, wrong success rate, wrong mechanic (BUG-4)
- [ ] **BUGGED**: ACD 1000ms → 2000ms (BUG-11)
- [ ] **BUGGED**: Cooldown 3000ms → 0 (BUG-12)

### Adaptation (1523) — CORRECT
- [x] SP: 1, 5-second first-use restriction, `cancelPerformance('adaptation')`

### Encore (1524) — CORRECT
- [x] SP: 1 + half of original, cooldown 10s, replays last performance

### Ensemble Skills (1550-1557) — CORRECTLY DEFERRED
- [x] All 8 defined with `isEnsemble: true`, deferred until party system

---

## FIX PLAN

### Phase A: Critical Formula Fixes (index.js only)

**A1: Wire bonusMaxSpPercent and bonusMaxHpPercent into getEffectiveStats**
- File: `server/src/index.js`, `getEffectiveStats()` (~line 2674)
- Add `+ (buffMods.bonusMaxSpPercent || 0)` to `bonusMaxSpRate`
- Add `+ (buffMods.bonusMaxHpPercent || 0)` to `bonusMaxHpRate`
- Fixes BOTH Service For You (Dancer) AND Apple of Idun (Bard)

**A2: Fix PDFM both formulas in calculatePerformanceEffects**
- ASPD: `5 + skillLevel * 3 + Math.floor(effectiveStats.dex / 10) + lessonsLv`
- Move: `5 + skillLevel * 3 + Math.floor(effectiveStats.agi / 10) + lessonsLv`

**A3: Fix Fortune's Kiss CRIT formula — add +10 base**
- `critBonus: 10 + skillLevel + Math.floor(effectiveStats.luk / 10) + lessonsLv`

**A4: Fix Service For You BOTH formulas — add full DL**
- MaxSP: `15 + skillLevel + Math.floor(effectiveStats.int / 10) + lessonsLv`
- SP reduction: `20 + skillLevel * 3 + Math.floor(effectiveStats.int / 10) + lessonsLv`

**A5: Fix Ugly Dance SP drain — add DL bonus**
- `spDrainPerTick: (5 + 5 * skillLevel) + (lessonsLv > 0 ? 5 + lessonsLv : 0)`

### Phase B: Charming Wink Complete Rewrite (index.js + data)

**B1: Fix skill data**
- `afterCastDelay: 2000` (was 1000)
- `cooldown: 0` (was 3000)

**B2: Rewrite handler — level-based charm on monsters**
- Monster success: `Math.max(0, Math.min(100, (player.baseLv - enemy.level) + 40))`
- On success: `enemy.charmedUntil = Date.now() + 10000`, `enemy.charmedBy = characterId`, clear aggro, set state idle
- In enemy AI tick: if `charmedUntil > Date.now()`, skip aggro/chase logic, optionally move toward charmer

### Phase C: Scream Delayed Execution (index.js)

**C1: Add 3-second delay to Scream stun application**
- Instead of applying stun immediately, use `setTimeout(3000)` to delay the stun check
- Store caster position at cast time; stun checks entities in screen radius from that position
- Same pattern should be applied to Bard's Frost Joker for consistency

### Phase D: Data Fixes (ro_skill_data_2nd.js)

**D1: Fix Moonlit Water Mill duration**
- Change `(24 + i * 4) * 1000` to `(15 + (i + 1) * 5) * 1000`

---

## Execution Order

1. **Phase A** (5 formula fixes) — Pure math corrections in `calculatePerformanceEffects` and `getEffectiveStats`. No new systems.
2. **Phase D** (1 data fix) — Simple data correction.
3. **Phase B** (2 fixes) — Charming Wink data + handler rewrite with enemy charm mechanic.
4. **Phase C** (1 fix) — Scream delayed execution (new pattern, also fixes Frost Joker).

---

## Post-Fix Verification Checklist

- [ ] Service For You MaxSP% actually increases player MaxSP (BUG-1)
- [ ] Apple of Idun MaxHP% actually increases player MaxHP (BUG-1)
- [ ] Fortune's Kiss Lv10/LUK99/DL10 gives +39 CRIT (was +29) (BUG-6)
- [ ] PDFM Lv10/DL10/DEX99 gives 54% ASPD reduction (was 49%) (BUG-2)
- [ ] PDFM Lv10/DL10/AGI99 gives 54% move speed reduction (was 34%) (BUG-3)
- [ ] Service For You Lv10/INT99/DL10 gives 44% MaxSP (was 34%) (BUG-7)
- [ ] Service For You Lv10/INT99/DL10 gives 69% SP reduction (was 64%) (BUG-8)
- [ ] Ugly Dance Lv5/DL10 drains 45 SP/tick (was 30) (BUG-5)
- [ ] Charming Wink success rate scales with level difference (BUG-4)
- [ ] Charming Wink makes monster follow caster for 10s (BUG-4)
- [ ] Charming Wink ACD is 2s, no cooldown (BUG-11, BUG-12)
- [ ] Moonlit Water Mill Lv1 duration is 20s (was 24s) (BUG-13)
- [ ] Scream stun fires after 3-second delay (was instant) (BUG-9)
- [ ] Humming Lv10/DL10/DEX99 gives +29 HIT (confirmed correct, no +1)
