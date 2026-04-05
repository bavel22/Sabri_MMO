# Priest Skills Comprehensive Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Priest_Class_Research](Priest_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-16
**Status:** AUDIT COMPLETE (v2 — deep research verified) — FIXES PENDING
**Scope:** All 19 Priest skills (IDs 1000-1018) vs canonical RO Classic pre-renewal
**Sources:** rAthena `battle.cpp`, `skill.cpp`, `status.cpp`, `pre-re/skill_db.yml` (GitHub master); iRO Wiki Classic; RateMyServer; rAthena issues #275, #947; `ro_ground_effects.js` AOE_RADIUS constants

---

## Executive Summary

**29 actionable issues across 12 skills. 19 critical, 10 moderate.** (C20 verified already correct)

The deep research (v2) uncovered 15 additional issues vs the initial audit:
- **Magnificat** doubles HP regen when it should only double SP regen (confirmed by rAthena issue #275 + Aegis testing)
- **Magnus Exorcismus** +30% bonus is renewal-only (must be removed), duration is 1s/lv too long, each wave should do SkillLv hits (not 1), Shadow element targeting is renewal-only
- **Turn Undead** formula uses 200*Lv instead of 20*Lv (10x too high), NO target stat subtraction (my v1 was wrong), cap is 70% not 100%, fail damage has erroneous *2 multiplier
- **Aspersio** has a 2000ms cast time that should be 0 (instant)
- **B.S. Sacramenti** has 3000ms cast time that should be 0, prerequisite is wrong, Holy Water catalyst is not in rAthena
- **Status Recovery** also needs to cure Sleep
- **Multiple skills** missing correct afterCastDelay values

Only 7 skills are fully correct.

---

## Skills With No Issues (7/19)

| ID | Skill | Verdict | Verification Notes |
|----|-------|---------|-------------------|
| 1001 | Kyrie Eleison | CORRECT | `floor(lv/2)+5` hits, `MaxHP*(10+2*lv)/100` barrier, blocks ALL physical (melee+ranged+skills), SP/cast/ACD all match rAthena |
| 1003 | Gloria | CORRECT | +30 LUK flat (all levels), durations 10/15/20/25/30s, SP 20, instant cast, ACD 2000 — all match |
| 1008 | Mace Mastery | CORRECT | +3 ATK/lv with maces in getPassiveSkillBonuses() |
| 1015 | Lex Divina | CORRECT | Silence toggle, boss immunity, SP array [20..10], duration array [30s..60s] — all match rAthena |
| 1016 | Inc SP Recovery | CORRECT | +3 SP regen/lv passive |
| 1017 | Safety Wall (Priest) | CORRECT | Hit count = `lv+1` (2-11), no HP durability in pre-renewal, aliased to Mage handler, Blue Gem catalyst |
| 1018 | Redemptio | STUB | Deferred until party system — correct |

---

## CRITICAL BUGS (18 issues across 8 skills)

### Turn Undead (1006) — 5 critical bugs

**C1: Formula uses 200*Lv instead of 20*Lv — gives 100% instead of ~43%**
**File:** `server/src/index.js` ~line 12601
**Current:** `(200 * learnedLevel + baseLv + intStat + lukStat) / 10` capped at 100%
**Canonical (rAthena `battle.cpp`):**
```c
i = 20 * skill_lv + sstatus->luk + sstatus->int_ + status_get_lv(src)
    + 200 - 200 * tstatus->hp / tstatus->max_hp;
if(i > 700) i = 700;  // Cap at 70%
// rnd()%1000 < i
```
**Impact:** At Lv10/BaseLv99/INT99/LUK30: current = 100% (always kills), correct = 42.8% at full HP

**C2: Missing boss immunity**
**Current:** No boss check — bosses can be instant-killed
**Canonical:** `MD_STATUSIMMUNE` flag prevents instant kill. Bosses always take fail damage only.

**C3: Missing HP-ratio modifier and 70% cap**
**Current:** No HP ratio, cap at 100%
**Canonical:** `+ 200 - 200 * TargetHP/TargetMaxHP` (up to +20% at 1 HP), hard cap 70%

**C4: Fail damage has erroneous *2 multiplier**
**File:** `server/src/index.js` ~line 12625
**Current:** `(baseLv + intStat + learnedLevel * 10) * 2`
**Canonical (rAthena):** `status_get_lv(src) + sstatus->int_ + skill_lv * 10` — NO *2
The *2 seen in wiki examples comes from the Holy element modifier vs Undead Lv4 (200%), not from the base formula.

**C5: Fail damage missing Holy element modifier**
**Current:** Flat damage, no element table lookup
**Canonical:** Holy element applied via `battle_attr_fix`. `IgnoreDefense: true` (ignores MDEF) but NOT `IgnoreElement`.
Pre-renewal `attr_fix.yml` Holy vs Undead values:
- vs Undead Lv1: **150%** (×1.5)
- vs Undead Lv2: **175%** (×1.75)
- vs Undead Lv3: **200%** (×2)
- vs Undead Lv4: **200%** (×2)

**Combined fix for Turn Undead handler (lines 12578-12655):**
```js
const tuStats = getEffectiveStats(player);
const baseLv = tuStats.level || 1;
const intStat = tuStats.int || 1;
const lukStat = tuStats.luk || 1;
const isBoss = tuEnemy.isBoss || tuEnemy.isMVP || false;
const enemyMaxHP = tuEnemy.maxHealth || 1;

// rAthena formula: 20*lv + LUK + INT + BaseLv + (1 - HP/MaxHP)*200, cap 700
let tuRate = 20 * learnedLevel + lukStat + intStat + baseLv
    + 200 - Math.floor(200 * tuEnemy.health / enemyMaxHP);
tuRate = Math.min(700, Math.max(0, tuRate)); // Cap at 70%

if (!isBoss && Math.random() * 1000 < tuRate) {
    // Instant kill (unchanged)
} else {
    // Fail damage: BaseLv + INT + Lv*10 (NO *2), then apply Holy element
    let tuDmg = Math.max(1, baseLv + intStat + learnedLevel * 10);
    const tuEleLevel = tuEnemy.element?.level || 1;
    const eleMod = getElementModifier('holy', tuEleType, tuEleLevel);
    tuDmg = Math.max(1, Math.floor(tuDmg * eleMod / 100));
    // ... rest of fail damage path
}
```

---

### Magnificat (1002) — 1 critical bug

**C6: Incorrectly doubles HP regen (should be SP regen ONLY)**
**File:** `server/src/index.js` ~line 12460 (handler) + ~line 18821 (HP regen tick)
**Current:** `hpRegenMultiplier: 2, spRegenMultiplier: 2` — doubles both HP and SP
**Canonical (rAthena `status.cpp`):** ONLY `regen->rate.sp += 100` for `SC_MAGNIFICAT`. No HP rate modification.
**Confirmed by:** rAthena issue #275 + Aegis testing: "HP regen shouldn't be affected by Magnificat"
**Impact:** HP regen is double what it should be during Magnificat
**Fix:**
1. Handler: Remove `hpRegenMultiplier: 2` from the buff application
2. HP regen tick (line 18821): Remove `if (hasBuff(player, 'magnificat')) hpRegen *= 2;`
3. `ro_buff_system.js` (line ~663): Remove `mods.hpRegenMultiplier` from magnificat case
4. Keep SP regen doubling (line 18871) and `mods.spRegenMultiplier` — that's correct

---

### Magnus Exorcismus (1005) — 5 critical bugs

**C7: AoE radius 350 → 175 (2x too large)**
**File:** `server/src/index.js` lines 12558, 12569
**Current:** `radius: 350` (14x14 cell area)
**Canonical:** 7x7 cells = `AOE_RADIUS['7x7']` = **175 UE**

**C8: +30% damage bonus is RENEWAL ONLY — must be removed**
**File:** `server/src/index.js` ~lines 20160-20165 (ground tick)
**Current:** `meDmg = Math.floor(meDmg * 1.3)` for Undead/Demon/Shadow/Undead-element
**Canonical (rAthena `battle.cpp`):** The `skillratio += 30` for `PR_MAGNUS` exists only inside `#ifdef RENEWAL`. In pre-renewal, ME simply does 100% MATK Holy damage to qualifying targets.
**Fix:** Remove the entire +30% bonus block.

**C9: Shadow element targeting is RENEWAL ONLY — must be removed**
**File:** `server/src/index.js` ~line 20118 (ground tick filter)
**Current:** Also targets Shadow-element enemies
**Canonical (rAthena `skill.cpp`):**
```c
#ifndef RENEWAL
    if (!battle_check_undead(tstatus->race, tstatus->def_ele) && tstatus->race != RC_DEMON)
        break;
#endif
```
Pre-renewal: Only Undead element OR Demon race. Shadow element is NOT a valid target.
**Fix:** Remove Shadow element from filter. Keep Undead element + Demon race only.

**C10: Duration off by 1 second per level**
**File:** `server/src/index.js` line 12551 + `ro_skill_data_2nd.js` (ME definition)
**Current:** `duration: 5000 + learnedLevel * 1000` → Lv1=6s, Lv10=15s
**Canonical:** Duration = `(4 + SkillLv)` seconds → Lv1=5s, Lv10=14s
**Fix:** Change to `4000 + learnedLevel * 1000`. Also fix in `ro_skill_data_2nd.js`: `duration: 4000 + (i+1) * 1000`

**C11: Each wave should do SkillLv hits (multi-hit), not 1 hit**
**File:** `server/src/index.js` ~line 20156 (ground tick damage calc)
**Current:** 1 damage event of 100% MATK per wave
**Canonical (rAthena `skill_db.yml`):** `HitCount` = SkillLv. Each wave fires SkillLv hits of 100% MATK.
At Lv10: each wave = 10 hits. Max 5 waves per target = 50 total hits.
**Impact:** ME damage at Lv10 is currently **10x too low** (500% MATK instead of 5000% MATK)
**Fix:** In the ground tick handler, after calculating single-hit MATK damage, multiply by learnedLevel:
```js
// Each wave does SkillLv hits of 100% MATK
const singleHitDmg = Math.max(1, meResult.damage || 0);
const meDmg = singleHitDmg * (effect.skillLevel || 1);
```
Broadcast the total damage as one event (same as how bolt spells bundle multi-hits).

---

### Sanctuary (1000) — 4 critical bugs

**C12: AoE radius 250 → 125 (2x too large)**
**File:** `server/src/index.js` lines 12391, 12403
**Current:** `radius: 250` (10x10 cell area)
**Canonical:** 5x5 cells = `AOE_RADIUS['5x5']` = **125 UE**

**C13: Missing Demon race damage**
**File:** `server/src/index.js` ~line 20065 (ground tick)
**Current:** `if (enemyEle !== 'undead') continue;`
**Canonical:** Damages Undead element AND Demon race enemies (both qualify independently)

**C14: Damage missing Holy element modifier**
**File:** `server/src/index.js` ~line 20063
**Current:** `const holyDmg = Math.floor(healPerTick / 2);` — flat damage
**Canonical:** Holy damage subject to element table via `battle_attr_fix`

**C15: Missing 2-cell knockback on damaged enemies**
**File:** `server/src/index.js` ~line 20078
**Current:** No knockback
**Canonical:** `Knockback: 2` in `skill_db.yml`, applied via `skill_attack_blow()` after damage

**C19: maxTargets is a LIFETIME damage counter, not per-tick heal limit**
**File:** `server/src/index.js` ~line 20040 (ground tick)
**Current:** `maxTargets` (val1 = `3 + skillLv`) is used to limit heals per tick (sort by distance, slice to maxTargets)
**Canonical (rAthena `skill.cpp` line 5821, 6923):**
- `val1 = skill_lv + 3` is a **lifetime damage hit budget**
- Each enemy damaged decrements val1 (`sg->val1 -= 1`)
- When val1 reaches 0, the **entire Sanctuary is destroyed** (line 12417: `skill_delunitgroup`)
- Healing has **NO per-tick limit** — ALL players in range are healed every tick
**Impact:** Sanctuary should self-destruct after (3+lv) damage hits, but currently lasts full duration regardless. Also healing is unnecessarily limited.
**Fix:**
```js
// In ground effect creation, store lifetime damage budget:
damageBudget: 3 + learnedLevel,
// In tick handler, decrement on each damage hit:
effect.damageBudget--;
if (effect.damageBudget <= 0) {
    removeGroundEffect(effect.id);
    broadcastToZone(sanctZone, 'skill:ground_effect_removed', { ... reason: 'hits_exhausted' });
    break;
}
// Remove maxTargets cap from healing loop (heal ALL players in range)
```

**C20: Lex Aeterna must NOT be consumed by DoT damage — VERIFIED CORRECT**
**File:** `server/src/index.js` lines 21215-21239 (status effect tick)
**Status:** ALREADY CORRECT. DoT drains go through `tickStatusEffects()` → `statusDrains` loop which has NO lex_aeterna check. Lex Aeterna is only checked in auto-attack and skill damage paths (lines 1407, 8465, 8603, 8763, 8864, 9010, etc.) — all within `battle_calc_damage`-equivalent code paths.
**No fix needed.**

---

### Status Recovery (1014) — 1 critical bug

**C16: Missing 'petrifying' AND 'sleep' from cleanse list**
**File:** `server/src/index.js` line 12920
**Current:** `cleanseStatusEffects(srTarget, ['freeze', 'stone', 'stun'])`
**Canonical (rAthena commit 7cc1cf0):** Cures SC_FREEZE, SC_STONE, SC_STONEWAIT (petrifying), SC_STUN, SC_SLEEP
**Fix:**
```js
const removed = cleanseStatusEffects(srTarget, ['freeze', 'stone', 'petrifying', 'stun', 'sleep']);
```

---

### Aspersio (1011) — 1 critical bug

**C17: Cast time 2000ms should be 0 (instant)**
**File:** `server/src/ro_skill_data_2nd.js` (Aspersio definition)
**Current:** `castTime: 2000`
**Canonical (rAthena `skill_db.yml`):** No CastTime entry = instant. The 2000ms is actually the AfterCastActDelay.
**Fix:** `castTime: 0, afterCastDelay: 2000`

---

### B.S. Sacramenti (1012) — 1 critical bug

**C18: Cast time 3000ms should be 0 (instant)**
**File:** `server/src/ro_skill_data_2nd.js` (BS Sacramenti definition)
**Current:** `castTime: 3000`
**Canonical (rAthena `skill_db.yml`):** No CastTime entry = instant.
**Fix:** `castTime: 0`

---

## MODERATE BUGS (10 issues across 7 skills)

### M1: Lex Aeterna (1007) — Cooldown should be ACD
**File:** `ro_skill_data_2nd.js`
**Current:** `cooldown: 3000` (not reducible by Bragi)
**Canonical:** `AfterCastActDelay: 3000` (IS reducible by Bragi)
**Fix:** `afterCastDelay: 3000, cooldown: 0`

### M2: Slow Poison (1013) — Reverses drain instead of preventing it
**File:** `server/src/index.js` ~line 18998
**Current:** Allows drain then reverses: `player.health += d.drain`
**Fix:** Move `slow_poison` check BEFORE the drain application so it's skipped entirely.

### M3: Slow Poison (1013) — Should restore natural HP/SP regen
**File:** `server/src/index.js` (regen tick)
**Current:** Poison blocks regen; Slow Poison doesn't unblock it
**Canonical (rAthena `status.cpp`):** Regen block checks `SC_POISON && !SC_SLOWPOISON`. While Slow Poison is active, natural regen resumes even though poison is still active.
**Fix:** In HP/SP regen tick, change poison regen block:
```js
// If poisoned but has Slow Poison, allow regen
if (hasStatus(player, 'poison') && !hasBuff(player, 'slow_poison')) {
    // block regen
}
```

### M4: Impositio Manus (1009) — Missing afterCastDelay
**File:** `ro_skill_data_2nd.js`
**Current:** `afterCastDelay: 0`
**Canonical (rAthena):** `AfterCastActDelay: 3000`
**Fix:** `afterCastDelay: 3000`

### M5: Suffragium (1010) — Missing afterCastDelay
**File:** `ro_skill_data_2nd.js`
**Current:** `afterCastDelay: 0`
**Canonical (rAthena):** `AfterCastActDelay: 2000`
**Fix:** `afterCastDelay: 2000`

### M6: Aspersio (1011) — Missing afterCastDelay
(Covered in C17 fix — castTime: 0, afterCastDelay: 2000)

### M7: Resurrection (1004) — afterCastDelay should vary by level
**File:** `ro_skill_data_2nd.js`
**Current:** `afterCastDelay: 0` at all levels
**Canonical (rAthena):** Lv1=0, Lv2=1000, Lv3=2000, Lv4=3000
**Fix:** Change to manual levels with `afterCastDelay: [0, 1000, 2000, 3000][i]`

### M8: B.S. Sacramenti (1012) — Prerequisite Aspersio Lv3 → Lv5
**File:** `ro_skill_data_2nd.js`
**Current:** `{ skillId: 1011, level: 3 }`
**Canonical (rAthena):** Aspersio Lv5
**Fix:** `{ skillId: 1011, level: 5 }`

### M9: B.S. Sacramenti (1012) — Remove Holy Water catalyst
**File:** `server/src/index.js` (SKILL_CATALYSTS)
**Current:** `bs_sacramenti: [{ itemId: 523, quantity: 1 }]`
**Canonical (rAthena `skill_db.yml`):** No ItemCost entry for B.S. Sacramenti
**Fix:** Remove `bs_sacramenti` from SKILL_CATALYSTS

### M10: Magnus Exorcismus (1005) — Remove Undead race from target filter
**File:** `server/src/index.js` ~line 20118
**Current:** `enemyRace !== 'undead'` included in target filter
**Canonical (rAthena pre-renewal):** `battle_check_undead()` with default config (`undead_detect_type: 0`) checks element only, not race. The race check is only for `RC_DEMON`.
**Fix:** Remove `enemyRace !== 'undead'` from the filter. Keep `enemyEle !== 'undead'` (element) and `enemyRace !== 'demon'` (race).

---

## IMPOSITIO MANUS ATK TYPE (Needs Verification)

**Potential issue:** rAthena uses `SCB_WATK` for Impositio Manus, meaning the bonus is weapon ATK (subject to size penalty and DEF reduction). Our implementation applies it as `bonusATK` in the buff system. Need to verify how `bonusATK` flows through the damage pipeline — if it bypasses DEF like mastery ATK, this is a discrepancy. Flagged for investigation but not blocking.

---

## COMPLETE FIX IMPLEMENTATION ORDER

### Phase A: Turn Undead (lines 12578-12655) — Full handler rewrite
1. **C1** — Fix formula: `20*Lv` not `200*Lv`
2. **C2** — Add boss immunity check (`isBoss || isMVP`)
3. **C3** — Add HP-ratio modifier + 70% cap
4. **C4** — Remove *2 from fail damage base formula
5. **C5** — Add `getElementModifier('holy', ...)` to fail damage

### Phase B: Magnificat (handler + regen tick + buff system)
6. **C6** — Remove `hpRegenMultiplier: 2` from handler, HP regen tick, and buff system

### Phase C: Magnus Exorcismus (placement handler + ground tick + skill data)
7. **C7** — Change `radius: 350` to `175` (lines 12558, 12569)
8. **C8** — Remove +30% damage bonus (renewal-only)
9. **C9** — Remove Shadow element from target filter
10. **C10** — Fix duration: `4000 + lv*1000` (not `5000 + lv*1000`)
11. **C11** — Multiply damage per wave by SkillLv (multi-hit)
12. **M10** — Remove Undead race from filter (keep element check only)

### Phase D: Sanctuary (placement handler + ground tick)
13. **C12** — Change `radius: 250` to `125` (lines 12391, 12403)
14. **C13** — Add Demon race to damage filter
15. **C14** — Apply Holy element modifier to damage
16. **C15** — Add 2-cell knockback via `knockbackTarget()`
17. **C19** — Add lifetime `damageBudget` counter (3+lv), destroy Sanctuary on exhaustion
18. **C19** — Remove per-tick heal limit (heal ALL players in range)

### Phase E: Skill Data Corrections (`ro_skill_data_2nd.js`)
17. **C17** — Aspersio: `castTime: 0, afterCastDelay: 2000` (was castTime: 2000)
18. **C18** — B.S. Sacramenti: `castTime: 0` (was castTime: 3000)
19. **M1** — Lex Aeterna: `afterCastDelay: 3000, cooldown: 0` (was cooldown: 3000)
20. **M4** — Impositio Manus: `afterCastDelay: 3000` (was 0)
21. **M5** — Suffragium: `afterCastDelay: 2000` (was 0)
22. **M7** — Resurrection: `afterCastDelay: [0, 1000, 2000, 3000][i]` (was 0)
23. **M8** — B.S. Sacramenti prerequisite: Aspersio Lv5 (was Lv3)
24. **C16** — ME duration fix in skill data: `4000 + (i+1) * 1000`

### Phase F: Status Recovery + Slow Poison + Catalysts
25. **C16** — Add 'petrifying' + 'sleep' to Status Recovery cleanse list
26. **M2** — Move Slow Poison check before poison drain
27. **M3** — Allow regen when poisoned + Slow Poison active
28. **M9** — Remove `bs_sacramenti` from SKILL_CATALYSTS
(C20 verified already correct — DoT does not consume Lex Aeterna)

---

## VERIFICATION CHECKLIST

### Turn Undead (1006)
- [ ] Non-Undead targets rejected with error
- [ ] Boss Undead: always fail damage, NEVER instant-killed
- [ ] Lv10/99/99/30 vs full HP Undead: ~42.8% success (NOT 100%)
- [ ] Same vs 1 HP Undead: ~62.8% (up to 70% cap)
- [ ] Fail damage = `BaseLv + INT + Lv*10` (NO *2)
- [ ] Fail damage applies Holy element table (Lv1=×1.5, Lv2=×1.75, Lv3-4=×2)
- [ ] SP deducted, aggro set, delays applied

### Magnificat (1002)
- [ ] SP regen doubled during buff
- [ ] HP regen NOT doubled (unchanged by buff)
- [ ] Duration: 30/45/60/75/90 seconds

### Magnus Exorcismus (1005)
- [ ] AoE radius = 175 UE (7x7 cells)
- [ ] Hits Undead-ELEMENT enemies only (not Undead race)
- [ ] Hits Demon-RACE enemies
- [ ] Does NOT hit Shadow-element enemies
- [ ] NO +30% bonus (100% MATK flat)
- [ ] Each wave = SkillLv hits (Lv10 = 10 hits per wave)
- [ ] Max 5 waves per target over 14s
- [ ] Duration: 5/6/7/8/9/10/11/12/13/14 seconds
- [ ] 3-second per-enemy immunity
- [ ] Blue Gemstone consumed

### Sanctuary (1000)
- [ ] AoE radius = 125 UE (5x5 cells)
- [ ] Heals ALL players in range per tick (no per-tick cap)
- [ ] Damages Undead-element enemies
- [ ] Damages Demon-race enemies
- [ ] Holy element modifier applied to damage (Holy vs Undead Lv1 = 150%)
- [ ] 2-cell knockback from center on damaged enemies
- [ ] Lifetime damage budget = 3 + skillLv (Sanctuary destroyed when exhausted)
- [ ] Blue Gemstone consumed

### Status Recovery (1014)
- [ ] Cures: freeze, stone, petrifying, stun, sleep
- [ ] Does NOT cure: silence, blind, curse, poison

### Aspersio (1011)
- [ ] Instant cast (0ms cast time)
- [ ] 2000ms after-cast delay
- [ ] Holy Water consumed

### B.S. Sacramenti (1012)
- [ ] Instant cast (0ms cast time)
- [ ] Prerequisite: Aspersio Lv5 (not Lv3)
- [ ] NO Holy Water consumption
- [ ] Holy armor endow applied

### Lex Aeterna (1007)
- [ ] After-cast delay 3s (reducible by Bragi), NOT cooldown
- [ ] NOT consumed by DoT (poison/bleeding ticks)
- [ ] Consumed only by battle_calc damage (auto-attacks, skills)

### Impositio Manus (1009)
- [ ] 3000ms after-cast delay

### Suffragium (1010)
- [ ] 2000ms after-cast delay

### Resurrection (1004)
- [ ] After-cast delay: Lv1=0, Lv2=1s, Lv3=2s, Lv4=3s

### Slow Poison (1013)
- [ ] Prevents poison drain (skips entirely, not reverse)
- [ ] Natural HP/SP regen resumes while active
- [ ] Poison timer keeps ticking independently

---

## KNOWN INTENTIONAL DEFERRALS (Not bugs)

| Skill | Deferral | Reason |
|-------|----------|--------|
| B.S. Sacramenti (1012) | Self-only instead of 3x3 ground AoE | Needs proximity + 2-Aco check |
| Magnificat (1002) | Self-only instead of party-wide | Needs party system |
| Gloria (1003) | Self-only instead of party-wide | Needs party system |
| Redemptio (1018) | Stub (no handler) | Needs party system |
| Status Recovery on Undead | No Blind infliction on Undead targets | Niche, low priority |
| Kyrie + Holy Light/Pressure | Not explicitly broken by these skills | Needs PvP or enemy-skill-on-player path |
| Impositio Manus ATK type | Applied as bonusATK (mastery) instead of wATK (subject to DEF) | Needs damage pipeline investigation |
| Kyrie vs magic damage | Magic doesn't deplete Kyrie barrier counter | Edge case, physical-only check is acceptable |
| Slow Poison self-cast | Allowed (correct per rAthena — no NoTargetSelf flag) | No issue |

---

## FORMULAS REFERENCE (rAthena verified)

### Turn Undead Instant Kill
```
rate = 20*SkillLv + LUK + INT + BaseLv + 200 - floor(200 * TargetHP / TargetMaxHP)
cap at 700 (70%)
roll: rnd()%1000 < rate
Bosses: IMMUNE (always fail damage)
```

### Turn Undead Fail Damage
```
damage = BaseLv + INT + SkillLv * 10  (NO *2 multiplier)
Apply Holy element modifier: damage * ElementTable[holy][targetElement][targetLevel] / 100
Ignores MDEF (IgnoreDefense: true)
Pre-renewal Holy vs Undead: Lv1=150%, Lv2=175%, Lv3=200%, Lv4=200%
Pre-renewal Holy vs Shadow: Lv1=125%, Lv2=150%, Lv3=175%, Lv4=200%
```

### Kyrie Eleison Barrier
```
barrierHP = floor(MaxHP * (10 + 2*SkillLv) / 100)
maxHits = floor(SkillLv / 2) + 5
Blocks: ALL physical damage (BF_WEAPON — melee, ranged, physical skills)
Does NOT block: magic damage
```

### Magnificat
```
Effect: Double SP recovery rate ONLY (NOT HP)
Duration: 30 + 15*Lv seconds
```

### Magnus Exorcismus (pre-renewal)
```
Target filter: Undead ELEMENT or Demon RACE (NOT Shadow, NOT Undead race)
Damage per wave: SkillLv hits × 100% MATK Holy
Wave interval: 3000ms per enemy immunity
Max waves per target: floor(duration/3) + 1  (5 at Lv10)
Duration: (4 + SkillLv) seconds
No damage bonus (100% flat — +30% is renewal-only)
```

### Sanctuary
```
Heal: [100,200,300,400,500,600,777,777,777,777] per tick (ALL players in range, no per-tick cap)
Lifetime damage budget: 3 + SkillLv (4-13 total damage hits before Sanctuary destroyed)
Undead/Demon damage: floor(heal/2) × ElementTable[holy][targetEle][targetLv] / 100
  Ignores MDEF (IgnoreDefense: true). Holy vs Undead Lv1 = 150% = effective 75% heal as damage
Knockback: 2 cells from center per damage hit
Duration: 4 + 3*Lv seconds (or until damage budget exhausted)
```

### Lex Aeterna
```
Consumed by: auto-attacks, physical skills, magical skills (battle_calc_damage only)
NOT consumed by: DoT (poison/bleeding), healing, misses, status effects, Shield Reflect
Duration: infinite until consumed or relog
ACD: 3000ms (reducible by Bragi). NOT a cooldown.
```
