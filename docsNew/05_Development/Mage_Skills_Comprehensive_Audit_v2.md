# Mage Skills Comprehensive Audit v2 — RO Classic Pre-Renewal Compliance

**Date**: 2026-03-16 (deep research verification pass)
**Scope**: All 14 Mage skills (IDs 200-213), every aspect audited against canonical RO Classic pre-renewal mechanics
**Sources**: irowiki.org/classic, ratemyserver.net, divine-pride.net, rAthena pre-re source (skill_db.yml, status.cpp, battle.cpp, pc.cpp, skill.cpp)
**Verification**: All 13 claims independently verified via rAthena source code cross-referenced with irowiki/ratemyserver

---

## Executive Summary

**14 skills audited. 11 bugs found (5 critical, 4 medium, 2 minor). 4 deferred items confirmed.**

| Severity | Count | Description |
|----------|-------|-------------|
| CRITICAL | 5 | Wrong formulas, missing mechanics that change gameplay |
| MEDIUM | 4 | Missing cross-system hooks, wrong sub-formulas |
| MINOR | 2 | Subtle behavioral differences (Thunderstorm re-check, Napalm split order) |
| DEFERRED | 4 | Fire Wall 1x3, boss knockback immunity, quest gating, range audit |
| CORRECT | 7 | Fire Ball, Stone Curse core, Sight, Napalm Beat, Soul Strike, Safety Wall core, Thunderstorm core |

---

## Per-Skill Audit Results

### 1. Cold Bolt (200) / Fire Bolt (201) / Lightning Bolt (202) — Shared Bolt Handler

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Water / Fire / Wind | Water / Fire / Wind | CORRECT |
| SP Cost | 10 + 2*Lv (12-30) | 12+i*2 (i=0-9) | CORRECT |
| Cast Time | 0.7*Lv sec (700-7000ms) | 700*(i+1) | CORRECT |
| ACD | 0.8 + 0.2*Lv sec (1000-2800ms) | 800+(i+1)*200 | CORRECT |
| Hits | 1 per level (1-10) | learnedLevel | CORRECT |
| Damage | 100% MATK per hit | effectValue: 100 | CORRECT |
| Range | 9 cells (450 UE) | 900 UE | ACCEPTABLE (doubled for gameplay, documented) |
| checkDamageBreakStatuses | Yes | Yes | CORRECT |
| Lex Aeterna | Double TOTAL damage (bundled), consume | NOT IMPLEMENTED | **BUG #1 (CRITICAL)** |
| Prerequisites | None | None | CORRECT |

**BUG #1**: Lex Aeterna is not checked in any magic skill handler. In rAthena, bolt skills are **bundled damage** — all hits are calculated as one total, and `battle_calc_damage()` doubles the entire bundle before display. Lex Aeterna is consumed after doubling. This also affects Soul Strike, Napalm Beat, Fire Ball, Thunderstorm, and Frost Diver (all bundled).

**rAthena source** (`battle.cpp` ~line 7649):
```cpp
if (tsc->getSCE(SC_AETERNA) && skill_id != PF_SOULBURN) {
    if (src->type != BL_MER || !skill_id)
        damage *= 2;  // Doubles entire bundled total
    status_change_end(bl, SC_AETERNA);
}
```
This runs in `battle_calc_damage()` which is called ONCE per skill use for bundled skills. For individual-hit skills (Storm Gust, LoV, Meteor Storm, Water Ball), it's called per-hit — so only the first hit doubles.

**Key classification**:
| Skill | Type | Lex Aeterna doubles... |
|-------|------|----------------------|
| Cold/Fire/Lightning Bolt | Bundled | ALL hits (total damage) |
| Soul Strike | Bundled | ALL hits (total damage) |
| Thunderstorm | Bundled | ALL hits (total damage) |
| Napalm Beat | Bundled (per-target after split) | Full per-target damage |
| Fire Ball | Bundled | Full damage |
| Frost Diver | Single hit | The hit |
| Storm Gust (Wizard) | Individual hits | First hit only |
| Lord of Vermilion (Wizard) | Individual hits | First hit only |
| Meteor Storm (Wizard) | Individual hits | First hit only |

---

### 2. Napalm Beat (203)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Ghost | Ghost | CORRECT |
| SP Cost | [9,9,9,12,12,12,15,15,15,18] | Same | CORRECT |
| Cast Time | 1000ms (rAthena pre-re skill_db.yml: CastTime: 1000) | 1000ms | CORRECT |
| ACD | [1000,1000,1000,900,900,800,800,700,600,500] | Same pattern | CORRECT |
| Damage | (70+10*Lv)% = 80-170% | effectValue: 80+i*10 | CORRECT |
| AoE | 3x3 cells (150 UE) | 150 UE | CORRECT |
| Damage Split | Per-target MDEF then divide by count | Per-target MDEF then divide | CORRECT |
| checkDamageBreakStatuses | Yes | Yes | CORRECT |
| Lex Aeterna | Should double + consume (bundled) | NOT CHECKED | See BUG #1 |
| Prerequisites | None | None | CORRECT |

**Napalm Beat damage split VERIFIED CORRECT**: rAthena has `SplashSplit: true` flag. In `battle.c`, the division happens AFTER per-target MDEF calculation: `ad.damage /= mflag`. Our implementation matches this.

**Napalm Beat cast time VERIFIED**: rAthena `pre-re/skill_db.yml` and `skill_cast_db.txt` both confirm 1000ms. The 500-600ms values are Renewal-only.

---

### 3. Increase SP Recovery (204) — Passive

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Flat SP regen | +3 SP per Lv per 10s tick | sprLv * 3 | CORRECT |
| MaxSP% regen | +0.2% MaxSP per Lv per 10s | floor(MaxSP * 0.002 * Lv) | CORRECT |
| SP item potency | **+10% per Lv** (10-100%) | +2% per Lv (2-20%) | **BUG #2 (CRITICAL)** |
| Prerequisites | None | None | CORRECT |

**BUG #2**: SP item potency bonus is +2% per level. Canonical RO Classic is **+10% per level**.

**rAthena source** (`pc.cpp`, `pc_itemheal` function):
```cpp
bonus = 100 + (sd->battle_status.int_ * 2)
      + pc_checkskill(sd, MG_SRECOVERY) * 10  // <-- +10 per level
      + pc_checkskill(sd, AM_LEARNINGPOTION) * 5;
```
The `* 10` on `MG_SRECOVERY` means +10% per level on a base-100 scale. At Lv10 = +100% = double SP item healing.

Applies to ALL SP-restoring items (handled in generic `pc_itemheal`), not just potions.

**Code location**: `server/src/index.js` line ~15557
**Current**: `spHeal = Math.floor(spHeal * (100 + isprLv * 2) / 100)`
**Fix**: `spHeal = Math.floor(spHeal * (100 + isprLv * 10) / 100)`

---

### 4. Sight (205)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| SP Cost | 10 | 10 | CORRECT |
| Cast Time | 0 | 0 | CORRECT |
| ACD | 0 | 0 | CORRECT |
| Duration | 10 seconds | 10000ms | CORRECT |
| Element | Fire | Fire | CORRECT |
| Effect | Reveal hidden in 7x7 area | `revealHidden: true`, AoE 700 UE | CORRECT |
| Detection tick | 1s proximity check, reveals hidden | Implemented in comprehensive audit (1s, 350 UE) | CORRECT |
| Reveals Hiding/Cloaking | Yes | Yes | CORRECT |
| Reveals Chase Walk (Stalker) | **No** — Chase Walk is immune to Sight | N/A (Stalker not implemented) | DEFERRED |
| Deals damage on reveal | **No** — only Ruwach deals damage | No damage | CORRECT |
| Prerequisites | None | None | CORRECT |

**VERDICT**: CORRECT

---

### 5. Stone Curse (206)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Earth | Earth | CORRECT |
| SP Cost | 26-Lv (25-16) | 25-i | CORRECT |
| Cast Time | 1000ms | 1000ms | CORRECT |
| Range | 2 cells (100 UE) | 100 UE | CORRECT |
| Stone Chance | (20+4*Lv)% = 24-60% | effectVal = 24+i*4 | CORRECT |
| Red Gemstone | Lv1-5 always, Lv6-10 success-only | Implemented | CORRECT |
| Mistress Card bypass | cardNoGemStone check | Implemented | CORRECT |
| Phase 1 (Petrifying) | 5s, can move, no atk/cast, CAN use items, NOT breakable | Correct flags set | CORRECT |
| Phase 2 (Stone) | 20s, full immobilize, DEF-50%, MDEF+25%, Earth Lv1, HP drain 1%/5s | Implemented | CORRECT |
| Stone blocks item use | YES (OPT1 block in rAthena) | NOT blocked in inventory:use | **BUG #3 (CRITICAL)** |
| Stone HP drain floor | Stops at **25% MaxHP** | Stops at 1 HP | **BUG #9 (MEDIUM)** |
| Auto-transition | petrifying→stone after 5s | transitionsTo: 'stone' | CORRECT |
| Counter-cast | Casting on petrifying target cures it | isPetrifying check, removes status | CORRECT |
| Boss/Undead immunity | Immune | applyStatusEffect handles | CORRECT |
| Prerequisites | None | None | CORRECT |

**BUG #3**: Stone, Freeze, Stun, and Sleep do NOT block item use. The `inventory:use` handler only checks `isHidden` and `isPlayDead`.

**rAthena source** (`pc.cpp` line ~6452, `pc_useitem`):
```cpp
if (nameid != ITEMID_NAUTHIZ && sd->sc.opt1 > 0
    && sd->sc.opt1 != OPT1_STONEWAIT   // Petrifying Phase 1 = ALLOWED
    && sd->sc.opt1 != OPT1_BURNING)     // Burning = ALLOWED
    return 0;  // Block item use
```
This blocks ALL OPT1 statuses **except** StoneWait (Petrifying Phase 1) and Burning.

**Complete list of statuses that must block item use**:
| Status | Mechanism | Currently Blocked? |
|--------|-----------|-------------------|
| Stone (Phase 2) | OPT1=1 | NO — **needs fix** |
| Freeze | OPT1=2 | NO — **needs fix** |
| Stun | OPT1=3 | NO — **needs fix** |
| Sleep | OPT1=4 | NO — **needs fix** |
| Hiding | NoConsumeItem | YES (already blocked) |
| Play Dead | NoConsumeItem | YES (already blocked) |
| Berserk (LK Frenzy) | NoConsumeItem | NO — **needs fix** (future) |
| Petrifying (Phase 1) | OPT1=6 EXEMPTED | CORRECT (not blocked) |

**BUG #9 (NEW)**: Stone HP drain stops at 1 HP in our code (`canKill: false` defaults to min 1 HP). Canonical RO Classic: drain stops at **25% MaxHP**.

**rAthena + irowiki**: "will reduce the target's HP by 1% every 5 seconds until only 25% HP remains"

**Fix**: In `ro_status_effects.js` stone config, set `periodicDrain.minHpPercent: 0.25`

---

### 6. Fire Ball (207)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Fire | Fire | CORRECT |
| SP Cost | 25 (all levels) | 25 | CORRECT |
| Cast Time | Lv1-5: 1500ms, Lv6-10: 1000ms | i<5?1500:1000 | CORRECT |
| ACD | Lv1-5: 1500ms, Lv6-10: 1000ms | Same | CORRECT |
| Damage | (70+10*Lv)% = 80-170% | effectValue: 80+i*10 | CORRECT |
| AoE | 5x5 cells (250 UE) | 250 UE | CORRECT |
| Full damage to all | Yes (no inner/outer ring in pre-re) | Yes | CORRECT |
| checkDamageBreakStatuses | Yes | Yes | CORRECT |
| Lex Aeterna | Doubles total (bundled) + consume | NOT CHECKED | See BUG #1 |
| Prerequisites | Fire Bolt Lv4 | Fire Bolt Lv4 | CORRECT |

**VERDICT**: CORRECT (except Lex Aeterna, covered by BUG #1)

---

### 7. Frost Diver (208)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Water | Water | CORRECT |
| SP Cost | 26-Lv (25-16) | 25-i | CORRECT |
| Cast Time | 800ms | 800ms | CORRECT |
| ACD | 1500ms | 1500ms | CORRECT |
| Damage | (100+10*Lv)% = 110-200% | effectValue: 110+i*10 | CORRECT |
| Freeze Chance | (35+3*Lv)% = 38-65% | 35 + learnedLevel * 3 | CORRECT |
| Freeze Duration | 3*Lv seconds **FLAT** (no stat reduction) | Reduced by MDEF and LUK | **BUG #4 (CRITICAL)** |
| Damage break before freeze | Yes | checkDamageBreakStatuses before freeze attempt | CORRECT |
| Lex Aeterna | Doubles damage + consume | NOT CHECKED | See BUG #1 |
| Prerequisites | Cold Bolt Lv5 | Cold Bolt Lv5 | CORRECT |

**BUG #4**: Frost Diver freeze duration is incorrectly reduced by target MDEF and LUK.

**rAthena source** (`status.cpp`, `status_change_start_sub`, SC_FREEZE):
```cpp
case SC_FREEZE:
    sc_def = st->mdef*100;                              // Reduces CHANCE
    sc_def2 = st->luk*10 + SCDEF_LVL_DIFF(bl,src,99,10); // Reduces CHANCE
    tick_def = 0;    // NO percentage duration reduction
#ifdef RENEWAL
    tick_def2 = status_get_luk(src) * -10;  // Renewal: caster LUK increases duration
#else
    tick_def2 = 0;   // Pre-renewal: NO duration reduction at all
#endif
    break;
```

In pre-renewal: `tick_def = 0` AND `tick_def2 = 0`. Freeze duration is **flat**: `3 * SkillLv` seconds. No stat reduces it. The only way to end freeze early is taking damage.

Note: irowiki classic says "Target's Magic Defense affects... duration" but this is **incorrect** per rAthena source. MDEF affects CHANCE only (`sc_def`), not duration.

**Code location**: `server/src/index.js` line ~7726
**Current**: `Math.max(1000, freezeBaseDuration - Math.floor(freezeBaseDuration * tarMdef / 200) - 10 * tarLuk)`
**Fix**: `const freezeDuration = freezeBaseDuration` (flat `learnedLevel * 3000`)

---

### 8. Fire Wall (209)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Fire | Fire | CORRECT |
| SP Cost | 40 | 40 | CORRECT |
| Cast Time | 2150-Lv*150 (2000-650ms) | 2150-(i+1)*150 | CORRECT |
| ACD | 0 | 0 | CORRECT |
| Hit Count | Lv+2 = 3-12 | effectValue: i+3 | CORRECT |
| Duration | (4+Lv)s = 5-14s | (5+learnedLevel-1)*1000 | CORRECT |
| Damage per hit | 50% MATK | 50% MATK in ground effect | CORRECT |
| Knockback | 2 cells per hit | 200 UE | CORRECT |
| Max concurrent | 3 per caster | 3 | CORRECT |
| Shape | 1x3 perpendicular to caster | Single-cell circle (150 radius) | DEFERRED |
| Per-cell hit counters | Each cell independent | Single shared counter | DEFERRED (tied to 1x3) |
| Boss hit rate | ~1 hit per 100ms (SKILLUNITTIMER_INTERVAL) | 300ms per-target immunity | **BUG #5 (MEDIUM)** |
| Boss non-knockback | Bosses aren't knocked back, eat charges fast | Not implemented | DEFERRED |
| Prerequisites | Fire Ball Lv5, Sight Lv1 | Same | CORRECT |

**BUG #5**: Fire Wall per-target immunity of 300ms is too slow for boss monsters.

**rAthena source**: `SKILLUNITTIMER_INTERVAL = 100` (100ms) in `skill.hpp`. Fire Wall unit interval = 20ms in skill_db. For boss monsters (knockback-immune), they consume charges at approximately 1 per 100ms (limited by SKILLUNITTIMER_INTERVAL). Our 300ms immunity means bosses only take ~3 hits/sec vs canonical ~10 hits/sec.

For normal monsters, the knockback-walkback cycle naturally spaces hits at ~470-700ms, so our 300ms immunity doesn't matter.

**Fix**: Reduce per-target immunity from 300ms to 100ms.

---

### 9. Soul Strike (210)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Ghost | Ghost | CORRECT |
| SP Cost | [18,14,24,20,30,26,36,32,42,38] zigzag | Same pattern | CORRECT |
| Cast Time | 500ms | 500ms | CORRECT |
| ACD | [1200,1000,1400,1200,1600,1400,1800,1600,2000,1800] zigzag | Same pattern | CORRECT |
| Hits | floor((Lv+1)/2) = 1,1,2,2,3,3,4,4,5,5 | Implemented | CORRECT |
| Damage | 100% MATK per hit | effectValue: 100 | CORRECT |
| Undead Bonus | +5% per SkillLv (all hits, bundled) | 1 + learnedLevel * 0.05 | CORRECT |
| Lex Aeterna | Doubles total (bundled) + consume | NOT CHECKED | See BUG #1 |
| Prerequisites | Napalm Beat Lv4 | Napalm Beat Lv4 | CORRECT |

**VERDICT**: CORRECT (except Lex Aeterna, covered by BUG #1)

---

### 10. Safety Wall (211)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| SP Cost | [30,30,30,35,35,35,40,40,40,40] | Same | CORRECT |
| Cast Time | [4000,3500,3000,2500,2000,1500,1000,1000,1000,1000] | Same | CORRECT |
| ACD | 0 | 0 | CORRECT |
| Hits Blocked | Lv+1 = 2-11 | effectValue: i+2 | CORRECT |
| Duration | 5*Lv seconds (5-50s) | (i+1)*5000 | CORRECT |
| Blue Gemstone | 1 per cast | In SKILL_CATALYSTS | CORRECT |
| Blocks melee auto-attacks | Yes | Auto-attack tick check | CORRECT |
| Blocks melee SKILLS | Yes (all BF_SHORT physical) | NOT IMPLEMENTED | **BUG #6 (MEDIUM)** |
| Blocks ranged attacks | No (BF_LONG not blocked) | N/A | CORRECT |
| Blocks magic | No (BF_MAGIC not blocked) | N/A | CORRECT |
| Overlap prevention (Pneuma) | Cannot coexist | Bidirectional check | CORRECT |
| Prerequisites | Napalm Beat Lv7, Soul Strike Lv5 | Same | CORRECT |

**BUG #6**: Safety Wall only blocks melee auto-attacks. Melee skills bypass it entirely.

**rAthena source** (`battle.cpp` ~line 3282, `battle_calc_damage`):
```cpp
if ((sce = sc->getSCE(SC_SAFETYWALL)) && (flag&(BF_SHORT|BF_MAGIC)) == BF_SHORT) {
    // Block damage, decrement hit counter
}
```
The check `(flag&(BF_SHORT|BF_MAGIC)) == BF_SHORT` means: must be short-range AND must NOT be magic. This is in the **generic damage function** — it blocks ALL melee physical attacks including skills.

**Known exceptions** (bypass Safety Wall):
- **Bowling Bash** — knockback resolves before damage, target leaves the cell
- **Clashing Spiral / Spiral Pierce** — explicitly bypasses (irowiki classic)

---

### 11. Thunderstorm (212)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| Element | Wind | Wind | CORRECT |
| SP Cost | 24+5*Lv (29-74) | 24+(i+1)*5 | CORRECT |
| Cast Time | Lv*1000ms (1000-10000) | (i+1)*1000 | CORRECT |
| ACD | 2000ms | 2000ms | CORRECT |
| Hits | Lv (1-10) | learnedLevel | CORRECT |
| Damage | 80% MATK per hit | effectVal: 80 | CORRECT |
| AoE | 5x5 cells (250 UE) | 250 UE | CORRECT |
| Full damage per target | Yes (not split) | Yes | CORRECT |
| AoE re-check per hit | Individual-hit style in rAthena | No (gathered once) | **MINOR #2** |
| checkDamageBreakStatuses | Yes | Yes | CORRECT |
| Lex Aeterna | Doubles total (bundled) + consume | NOT CHECKED | See BUG #1 |
| Prerequisites | Lightning Bolt Lv4 | Lightning Bolt Lv4 | CORRECT |

**MINOR #2**: Thunderstorm in rAthena fires individual skill unit hits. Our implementation gathers enemies once. With 200ms stagger, the practical difference is negligible.

---

### 12. Energy Coat (213)

| Aspect | Expected (RO Classic) | Implemented | Status |
|--------|----------------------|-------------|--------|
| SP Cost | 30 | 30 | CORRECT |
| Cast Time | 5000ms | 5000ms | CORRECT |
| ACD | 0 | 0 | CORRECT |
| Duration | 300s (5 min) | 300000ms | CORRECT |
| Damage reduction tiers | 6/12/18/24/30% | 0.06-0.30 | CORRECT |
| Dispel at 0 SP | Yes | Yes (removeBuff) | CORRECT |
| Reduces ALL physical (BF_WEAPON) | Melee + ranged + skills | Only enemy auto-attack | **BUG #7 (MEDIUM)** |
| Reduces magical damage | No (pre-renewal: BF_WEAPON only) | N/A (not called) | CORRECT |
| SP drain formula | **(10+5*tier) * MaxSP / 1000** | damage * drainPct | **BUG #8 (MEDIUM)** |
| Cart Termination bypass | Yes (WS_CARTTERMINATION exempt) | N/A | DEFERRED |
| Quest skill gating | Requires quest | No quest system | DEFERRED |
| Prerequisites | None (quest) | None | CORRECT |

**BUG #7**: `applyEnergyCoat()` is only called in the enemy auto-attack tick. In pre-renewal, Energy Coat reduces **ALL BF_WEAPON damage** — melee auto-attacks, ranged auto-attacks, Bash, Pierce, Double Strafe, Arrow Shower, etc.

**rAthena source** (`battle.cpp` ~line 3397):
```cpp
if (tsc->getSCE(SC_ENERGYCOAT) && (flag&BF_WEAPON && skill_id != WS_CARTTERMINATION)) {
    // Applies to ALL weapon-type damage (melee + ranged, auto + skills)
}
```
The check is `flag&BF_WEAPON` with no `BF_SHORT`/`BF_LONG` distinction. All physical/weapon damage triggers it.

**BUG #8 (NEW)**: Energy Coat SP drain uses wrong formula. Our code drains `damage * spDrainPct` (damage-based). Canonical formula is MaxSP-based.

**rAthena source**:
```cpp
int32 per = 100*status->sp / status->max_sp - 1;
per /= 20;  // tier 0-4
if (!status_charge(bl, 0, (10+5*per)*status->max_sp/1000))
    status_change_end(bl, SC_ENERGYCOAT);
damage -= damage * 6 * (1 + per) / 100;
```

| SP % | Tier | Damage Reduction | SP Drain (rAthena) | SP Drain (our code) |
|------|------|------------------|--------------------|---------------------|
| 81-100% | 4 | 30% | 3.0% of **MaxSP** | 3.0% of **damage** |
| 61-80% | 3 | 24% | 2.5% of **MaxSP** | 2.5% of **damage** |
| 41-60% | 2 | 18% | 2.0% of **MaxSP** | 2.0% of **damage** |
| 21-40% | 1 | 12% | 1.5% of **MaxSP** | 1.5% of **damage** |
| 1-20% | 0 | 6% | 1.0% of **MaxSP** | 1.0% of **damage** |

These produce very different results: high-damage hits drain MORE SP in our code but should drain a **fixed** amount based on MaxSP. Example: Player with 5000 MaxSP at 90% SP, takes a 2000 damage hit:
- Our code: drains `2000 * 0.03 = 60 SP`
- Canonical: drains `(10+5*4) * 5000 / 1000 = 150 SP`

---

## Bug Summary & Fix Plan

### Phase A: Critical Formula Fixes (5 bugs)

**BUG #1 — Lex Aeterna on bundled magic skills** (CRITICAL)
- **File**: `server/src/index.js`
- **Scope**: Bolt handler (200/201/202), Soul Strike (210), Napalm Beat (203), Fire Ball (207), Thunderstorm (212), Frost Diver (208)
- **Fix**: After calculating `totalDamage` in the hit loop but BEFORE applying damage, check for Lex Aeterna on the target. If present, double `totalDamage` and remove the debuff.
- **IMPORTANT**: These are BUNDLED skills — Lex doubles the TOTAL, not just first hit. The implementation should be:
  ```javascript
  // After hit loop calculates totalDamage:
  if (hasStatusEffect(target, 'lex_aeterna')) {
      totalDamage *= 2;
      removeStatusEffect(target, 'lex_aeterna');
      // Also double each hitDamages[i] proportionally for display
      for (let i = 0; i < hitDamages.length; i++) hitDamages[i] *= 2;
  }
  ```
- **For Napalm Beat**: Apply per-target (after split calculation, before applying to target)
- **Pattern**: Storm Gust/LoV/Meteor Storm (Wizard) already implement first-hit-only Lex correctly. Bundled skills need total-damage Lex.
- **Testing**: Lex Aeterna + Fire Bolt Lv10 → ALL 10 hits doubled (total = 20x MATK). Lex consumed.

**BUG #2 — Inc SP Recovery item potency** (CRITICAL)
- **File**: `server/src/index.js` line ~15557
- **Change**: `isprLv * 2` → `isprLv * 10`
- **Testing**: Lv10 Inc SP Recovery + Blue Potion → should restore 2x base heal (was 1.2x)

**BUG #3 — Status effects blocking item use** (CRITICAL)
- **File**: `server/src/index.js` in `inventory:use` handler (line ~15367)
- **Fix**: Add checks for these statuses from `getCombinedModifiers()`:
  ```javascript
  const mods = getCombinedModifiers(player);
  if (mods.isStoned) return error('Cannot use items while petrified');
  if (mods.isFrozen) return error('Cannot use items while frozen');
  if (mods.isStunned) return error('Cannot use items while stunned');
  if (mods.isSleeping) return error('Cannot use items while asleep');
  // isHidden and isPlayDead already blocked
  // isPetrifying (Phase 1) must NOT be blocked — players CAN use items
  ```
- **Testing**: Stone (Phase 2) → potion fails. Freeze → potion fails. Petrifying (Phase 1) → potion succeeds.

**BUG #4 — Frost Diver duration reduction** (CRITICAL)
- **File**: `server/src/index.js` line ~7726
- **Change**: Remove MDEF/LUK duration reduction. Use flat duration.
  ```javascript
  // BEFORE (wrong):
  const freezeDuration = Math.max(1000, freezeBaseDuration - Math.floor(freezeBaseDuration * tarMdef / 200) - 10 * tarLuk);
  // AFTER (correct):
  const freezeDuration = freezeBaseDuration; // flat 3*Lv seconds
  ```
- **Why**: rAthena pre-renewal: `tick_def = 0`, `tick_def2 = 0`. Duration flat. MDEF/LUK only reduce CHANCE.
- **Testing**: Frost Diver Lv10 on high-MDEF target → freeze lasts 30s (not reduced)

**BUG #9 — Stone HP drain floor** (CRITICAL)
- **File**: `server/src/ro_status_effects.js` stone config (~line 50-70)
- **Change**: Set `periodicDrain.minHpPercent: 0.25` (was `null` → defaulting to 1 HP)
- **Why**: irowiki + rAthena: "reduce HP by 1% every 5 seconds until only 25% HP remains"
- **Testing**: Get stoned at 100% HP → HP drains to 25% and stops. Does NOT drain below 25%.

### Phase B: Cross-System Hooks (4 bugs)

**BUG #5 — Fire Wall boss hit rate** (MEDIUM)
- **File**: `server/src/index.js` in Fire Wall ground effect tick handler
- **Change**: Reduce per-target immunity from 300ms to 100ms
- **Why**: rAthena SKILLUNITTIMER_INTERVAL = 100ms. Bosses consume charges at ~1/100ms.
- **Testing**: Boss in Fire Wall Lv10 → 12 charges consumed in ~1.2s (was ~3.6s)

**BUG #6 — Safety Wall melee skill blocking** (MEDIUM)
- **File**: `server/src/index.js`
- **Fix**: Create `checkSafetyWallBlock(target)` helper, call at start of all melee physical skill handlers.
- **Exceptions**: Bowling Bash (701) and Spiral Pierce (if implemented) should NOT check SW.
- **Affected melee skills**: Bash (100), Magnum Break (105), Pierce (700), Brandish Spear (702), Sonic Blow (1100), Back Stab (1700), Raid (1701), Cart Revolution (603), Triple Attack/Chain Combo/Combo Finish (Monk), Mammonite (600), Hammer Fall (1209), and all other melee physical skills
- **Testing**: Stand on SW → Bash blocked, Pierce blocked, Bowling Bash NOT blocked

**BUG #7 — Energy Coat scope** (MEDIUM)
- **File**: `server/src/index.js`
- **Fix**: Call `applyEnergyCoat()` in ALL physical damage paths that target players (BF_WEAPON flag), not just auto-attack tick. This includes melee skills AND ranged physical skills.
- **Cart Termination exception**: When implemented, skip Energy Coat for WS_CARTTERMINATION
- **Testing**: Energy Coat active → Bash damage reduced. Double Strafe damage reduced. Magic NOT reduced.

**BUG #8 — Energy Coat SP drain formula** (MEDIUM)
- **File**: `server/src/index.js` in `applyEnergyCoat()` function (line ~1016)
- **Change**: SP drain from damage-based to MaxSP-based
  ```javascript
  // BEFORE (wrong — damage-based):
  const spDrain = Math.max(1, Math.floor(damage * spDrainPct));

  // AFTER (correct — MaxSP-based):
  const tier = spRatio > 0.8 ? 4 : spRatio > 0.6 ? 3 : spRatio > 0.4 ? 2 : spRatio > 0.2 ? 1 : 0;
  const spDrain = Math.max(1, Math.floor((10 + 5 * tier) * player.maxMana / 1000));
  ```
- **Testing**: 5000 MaxSP at 90% SP, any physical hit → drains 150 SP (not damage-dependent)

### Phase C: Minor Behavioral Differences (2 issues — WONTFIX)

**MINOR #1 — Napalm Beat split order** — VERIFIED CORRECT per rAthena `SplashSplit` flag. No change needed.

**MINOR #2 — Thunderstorm per-hit AoE re-check** — negligible with 200ms stagger. No change needed.

### Deferred Items (Unchanged)

| Item | Blocked By | Notes |
|------|-----------|-------|
| Fire Wall 1x3 perpendicular placement | Directional math | Would also enable per-cell hit counters |
| Fire Wall boss non-knockback | Boss mode flag in ground effect | Bosses walk through, eating charges fast |
| Energy Coat cast interruptibility | Cast interruption system | Need uninterruptible flag |
| Quest skill gating (Energy Coat) | Quest system | Low priority |
| Magic range audit (900→450 UE) | Coordinated change | Deferred until range rebalance |
| Chase Walk immunity to Sight | Stalker class not implemented | Future |
| Cart Termination EC bypass | Blacksmith skill | Future |

---

## Verification: Confirmed Correct Implementations

These aspects were verified against rAthena source code as matching RO Classic:

1. **Bolt multi-hit** — SP, cast time, ACD, hits, damage all match rAthena pre-re
2. **Napalm Beat** — Cast time 1000ms confirmed, damage split per-target-then-divide matches SplashSplit
3. **Fire Ball AoE** — 250 UE (5x5), no inner/outer ring in pre-re, full damage to all
4. **Thunderstorm damage** — 80% MATK per hit matches rAthena pre-re effectValue
5. **Stone Curse two-phase** — petrifying→stone transition, counter-cast, gem rules all correct
6. **Frost Diver freeze chance** — raw chance to applyStatusEffect, no double-MDEF
7. **Soul Strike zigzag** — SP and ACD zigzag patterns match exactly
8. **Energy Coat tier percentages** — 6/12/18/24/30% reduction matches `6*(1+tier)/100`
9. **Safety Wall overlap prevention** — Pneuma bidirectional check correct
10. **Fire Wall max 3 concurrent** — matches rAthena CASTER_LIMITS

---

## Testing Checklist

### Phase A Tests (Critical)
- [ ] Lex Aeterna + Fire Bolt Lv10: ALL 10 bolts doubled, Lex consumed after
- [ ] Lex Aeterna + Soul Strike Lv10: ALL 5 hits doubled (bundled)
- [ ] Lex Aeterna + Napalm Beat: per-target damage doubled (after split)
- [ ] Lex Aeterna + Thunderstorm Lv5: ALL 5 bolts doubled (bundled)
- [ ] Lex Aeterna + Frost Diver: damage doubled, then freeze applied
- [ ] Lex Aeterna + Fire Ball: primary + all splash targets doubled
- [ ] Inc SP Recovery Lv10 + Blue Potion: heal = 2x base (was 1.2x)
- [ ] Stone (Phase 2) → try potion → should FAIL
- [ ] Freeze → try potion → should FAIL
- [ ] Stun → try potion → should FAIL
- [ ] Sleep → try potion → should FAIL
- [ ] Petrifying (Phase 1) → try potion → should SUCCEED
- [ ] Frost Diver Lv10 on high-MDEF target → freeze lasts 30s flat
- [ ] Frost Diver Lv5 → freeze lasts 15s flat
- [ ] Stone Curse → HP drains to 25% MaxHP → stops (not to 1 HP)

### Phase B Tests (Medium)
- [ ] Fire Wall + boss monster → charges consumed ~1 per 100ms
- [ ] Fire Wall + normal monster → knockback, hits at normal rate
- [ ] Safety Wall + Bash → BLOCKED, SW charge consumed
- [ ] Safety Wall + Pierce → BLOCKED
- [ ] Safety Wall + Bowling Bash → NOT blocked (exception)
- [ ] Safety Wall + ranged attack → NOT blocked
- [ ] Safety Wall + magic spell → NOT blocked
- [ ] Energy Coat + Bash → damage reduced by tier %
- [ ] Energy Coat + Double Strafe → damage reduced (ranged physical)
- [ ] Energy Coat + magic spell → NOT reduced
- [ ] Energy Coat SP drain: 5000 MaxSP at 90%, any hit → drains 150 SP exactly

---

## Implementation Priority

1. **Phase A** (5 critical bugs) — Fix formula/mechanic bugs. Direct gameplay balance impact.
   - BUG #1: Lex Aeterna on bundled magic skills
   - BUG #2: Inc SP Recovery item potency +2% → +10%
   - BUG #3: Stone/Freeze/Stun/Sleep block item use
   - BUG #4: Frost Diver flat duration (no stat reduction)
   - BUG #9: Stone HP drain floor 1 HP → 25% MaxHP

2. **Phase B** (4 medium bugs) — Add cross-system hooks. Affects defensive spell utility.
   - BUG #5: Fire Wall immunity 300ms → 100ms
   - BUG #6: Safety Wall blocks melee skills
   - BUG #7: Energy Coat reduces all BF_WEAPON damage
   - BUG #8: Energy Coat SP drain MaxSP-based formula

3. **Phase C** (2 minor) — No code changes needed.

4. **Deferred** — Implement when respective systems are built.

**Estimated scope**: Phase A = ~60 lines, Phase B = ~100 lines (helper functions + hooks in skill handlers)
