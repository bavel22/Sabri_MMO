# Wizard Skills Comprehensive Audit & Fix Plan (v2 — Deep Research)

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Wizard_Class_Research](Wizard_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date**: 2026-03-16
**Scope**: All 14 Wizard skills (IDs 800-813) — data, handlers, ground effect ticks, status effects
**Sources**: rAthena pre-re/skill_db.txt, pre-re/skill_cast_db.txt, Hercules skill_db.conf, battle.c, iROwiki Classic, RateMyServer, Divine Pride
**Files**: `server/src/index.js`, `server/src/ro_skill_data_2nd.js`

---

## Audit Summary

| Category | Count |
|----------|-------|
| CRITICAL bugs (broken mechanics) | 5 |
| HIGH priority (wrong damage/behavior) | 6 |
| DATA fixes (ro_skill_data_2nd.js) | 13 |
| MEDIUM handler fixes | 6 |
| LOW / deferred | 5 |
| **Total issues** | **35** |

---

## CRITICAL BUGS — Status Effects Completely Non-Functional

All four ground AoE / self-AoE Wizard skills call `applyStatusEffect()` with the **wrong argument signature**. The function expects `(source, target, statusType, baseChance, overrideDuration)` but the handlers pass `(target, statusType, optionsObject)`. Result: `calculateResistance()` receives a non-string statusType, silently returns `{ applied: false }`, and **no status effect is ever applied**.

### C1. Storm Gust freeze NEVER applies
**Line**: 19663
**Current**: `applyStatusEffect(enemy, 'freeze', { baseLevelSrc: sgCaster.level || 1, ... })`
**Fix**: `applyStatusEffect(sgCaster, enemy, 'freeze', 150, 12000)`
**Note**: 150% base chance is canonical (almost guaranteed, reduced by target MDEF).

### C2. Lord of Vermilion blind NEVER applies
**Line**: 19717
**Current**: `applyStatusEffect(enemy, 'blind', { baseLevelSrc: lovCaster.level || 1, ... })`
**Fix**: `applyStatusEffect(lovCaster, enemy, 'blind', 100, 30000)` (handler pre-rolls chance)

### C3. Meteor Storm stun NEVER applies
**Line**: 19762
**Current**: `applyStatusEffect(enemy, 'stun', { baseLevelSrc: msCaster.level || 1, ... })`
**Fix**: `applyStatusEffect(msCaster, enemy, 'stun', 100, 5000)` (handler pre-rolls chance)

### C4. Frost Nova freeze NEVER applies
**Line**: 11303
**Current**: `applyStatusEffect(enemy, 'freeze', { baseLevelSrc: player.level || 1, ... })`
**Fix**: `applyStatusEffect(player, enemy, 'freeze', 100, fnFreezeDuration)` where `fnFreezeDuration = 1500 * learnedLevel`

### C5. Sight Blaster reactive trigger COMPLETELY MISSING
**Lines**: 11342-11361
**Bug**: Buff applied but NO proximity check in any server tick loop. Skill is 100% non-functional.
**Fix**: Add proximity trigger check (~1s tick) for players with `sight_blaster` buff. When enemy enters 3x3 area (75 UE): deal 100% MATK Fire, knockback 3 cells, remove buff.

---

## HIGH PRIORITY — Wrong Damage/Behavior

### H1. Storm Gust: frozen targets should be IMMUNE to further ticks
**Lines**: 19629-19636
**Bug**: Current code deals damage to frozen enemies. Canonical: once a target is frozen by Storm Gust, it stops taking further damage ticks until the freeze breaks. This is a core design feature — bosses/undead (who can't freeze) take ALL 10 ticks = max damage, while regular monsters freeze after 3 hits and take much less.
**Fix**: Add at top of per-enemy loop in storm_gust tick:
```js
if (enemy.activeStatusEffects?.has('freeze')) continue; // Frozen = immune to SG
```

### H2. Storm Gust AoE radius wrong — should be 225 (9x9), not 175 (7x7)
**Lines**: 10996, 19627
**Source**: rAthena pre-re skill_db — Storm Gust splash range is 4 (= 9x9 area). With implicit 3x3 sub-splash, effective area is 11x11.
**Fix**: Change `radius: 175` → `radius: 225` in both the placement handler (10996) and tick handler fallback (19627).

### H3. Quagmire AGI/DEX reduction is HALF of canonical
**Data line 49, handler lines 11092/11117/19875**
**Bug**: effectValue `5*(i+1)` = [5,10,15,20,25]. Canonical (rAthena): `10 * level` = [10,20,30,40,50].
**Fix**: Change data to `effectValue: 10*(i+1)`. Update handler to use new values.

### H4. Quagmire: bosses should NOT be immune
**Line**: 19868
**Bug**: `if (enemy.modeFlags?.statusImmune) continue;` skips bosses. Canonical: Quagmire works on ALL monsters including MVPs/bosses.
**Fix**: Remove the `statusImmune` check from the Quagmire tick handler. (Keep it for Storm Gust/LoV/Meteor Storm where boss immunity IS correct.)

### ~~H5. Heaven's Drive MATK% should be 100%, not 125%~~ — REVERTED
**Status**: REVERTED. Verification via iROWiki and rAthena issue #4834 confirms Heaven's Drive IS 125% MATK per hit. The original code was correct. The initial research was wrong. Reverted back to 125%.

### H6. Meteor Storm per-meteor splash should be 150 UE (7x7), not 75 UE (3x3)
**Lines**: 11067, 19745
**Source**: rAthena skill_db — Meteor Storm splash = 3 cells from center = 7x7 area per meteor. Current `meteorRadius: 75` is only 3x3.
**Fix**: Change `meteorRadius: 75` → `meteorRadius: 150` in placement (11067) and verify tick handler uses `effect.meteorRadius` (line 19745).

---

## DATA FIXES — ro_skill_data_2nd.js

All corrections to `server/src/ro_skill_data_2nd.js` lines 42-56:

### D1. Earth Spike (804) — cast time
**Current**: `castTime: 700*(i+1)` → [700, 1400, 2100, 2800, 3500]
**Canonical (rAthena)**: [1000, 2000, 3000, 4000, 5000] = `1000*(i+1)`

### D2. Earth Spike (804) — after-cast delay
**Current**: `afterCastDelay: 0`
**Canonical**: `afterCastDelay: 700`

### D3. Heaven's Drive (805) — after-cast delay
**Current**: `afterCastDelay: 0`
**Canonical**: `afterCastDelay: 700`

### D4. Quagmire (806) — after-cast delay
**Current**: `afterCastDelay: 0`
**Canonical**: `afterCastDelay: 1000`

### D5. Quagmire (806) — effectValue (AGI/DEX reduction)
**Current**: `effectValue: 5*(i+1)` → [5, 10, 15, 20, 25]
**Canonical**: `effectValue: 10*(i+1)` → [10, 20, 30, 40, 50]

### D6. Ice Wall (808) — duration
**Current**: `duration: 5000*(i+1)+5000` → [10000, 15000, ..., 55000]
**Canonical**: `duration: 5000*(i+1)` → [5000, 10000, ..., 50000]
Off by +5000ms at every level.

### D7. Sight Rasher (809) — ACD/cooldown swapped
**Current**: `afterCastDelay: 0, cooldown: 2000`
**Canonical**: `afterCastDelay: 2000, cooldown: 0`

### D8. Fire Pillar (810) — after-cast delay
**Current**: `afterCastDelay: 0`
**Canonical**: `afterCastDelay: 1000`

### D9. Frost Nova (811) — cast time array
**Current**: `[6000, 5600, 5200, 4800, 4400, 4000, 4000, 4000, 4000, 4000]`
**Canonical (rAthena)**: `[6000, 6000, 5500, 5500, 5000, 5000, 4500, 4500, 4000, 4000]` (stepped pairs)

### D10. Frost Nova (811) — MATK% (effectValue)
**Current**: `73+i*7` → [73, 80, 87, 94, 101, 108, 115, 122, 129, 136]
**Canonical**: `floor((100+(i+1)*10)*2/3)` → [73, 80, 87, 93, 100, 107, 113, 120, 127, 133]
Use explicit array: `[73, 80, 87, 93, 100, 107, 113, 120, 127, 133]`

### D11. Frost Nova (811) — after-cast delay
**Current**: `afterCastDelay: 0`
**Canonical**: `afterCastDelay: 1000`

### D12. Lord of Vermilion (801) / Storm Gust (803) — ACD vs cooldown swapped
**Current**: `afterCastDelay: 0, cooldown: 5000`
**Canonical**: `afterCastDelay: 5000, cooldown: 0`
ACD blocks ALL skills for 5s. Cooldown only blocks the same skill. These are in the wrong field.

### D13. Meteor Storm (802) — after-cast delay non-linear
**Current**: `afterCastDelay: 2500+i*500` → [2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000]
**Canonical (rAthena)**: `[2000, 3000, 3000, 4000, 4000, 5000, 5000, 6000, 6000, 7000]`
Use explicit array.

---

## MEDIUM HANDLER FIXES

### M1. Lex Aeterna not consumed by ground AoE ticks
**Lines**: Storm Gust ~19636, LoV ~19695, Meteor Storm ~19757
**Bug**: Ground AoE tick damage doesn't check/consume Lex Aeterna on targets.
**Fix**: Add per-target Lex Aeterna check (same pattern as Heaven's Drive at line 10876).

### M2. Lex Aeterna not consumed by Sight Rasher / Frost Nova
**Lines**: Sight Rasher ~11231, Frost Nova ~11281
**Fix**: Add Lex Aeterna check inside the per-enemy AoE loop.

### M3. Fire Pillar max 5 should auto-remove oldest, not error
**Line**: 11143
**Fix**: Replace `socket.emit('skill:error')` with oldest-removal logic (same pattern as Quagmire).

### M4. Fire Pillar missing checkDamageBreakStatuses
**Line**: ~19968
**Fix**: Add `checkDamageBreakStatuses(enemy)` after damage application.

### M5. Quagmire strip list incomplete
**Line**: 19890
**Current**: `['two_hand_quicken', 'spear_quicken', 'increase_agi']`
**Canonical adds**: `'adrenaline_rush'`, `'improve_concentration'` (both implemented in game)
**Future adds** (when implemented): `'one_hand_quicken'`, `'wind_walker'`, `'cart_boost'`, `'true_sight'`

### M6. Frost Nova freeze duration should scale per level
**Line**: 11303 (and C4 fix)
**Current**: Hardcoded 12000ms
**Canonical**: `1500 * SkillLv` → [1500, 3000, 4500, 6000, 7500, 9000, 10500, 12000, 13500, 15000]
**Fix**: When fixing C4, use `1500 * learnedLevel` as the override duration.

### M7. Sense missing element level
**Line**: 11328
**Fix**: Add `elementLevel: senseEnemy.elementLevel || 1` to the `skill:sense_result` payload.

### M8. Storm Gust missing 2-cell knockback per tick
**Source**: rAthena skill_db knockback column = 2 for Storm Gust.
**Current**: No knockback in Storm Gust tick handler.
**Fix**: After each non-freezing Storm Gust tick on an enemy, push them 2 cells away from center using `knockbackTarget()`. Skip if boss (knockback immune) or if frozen.

### M9. Meteor Storm meteor count formula wrong
**Line**: 11056
**Current**: `Math.floor((learnedLevel + 2) / 2)` → [1, 2, 2, 3, 3, 4, 4, 5, 5, 6]
**Canonical (rAthena)**: [2, 3, 3, 4, 4, 5, 5, 6, 6, 7]
**Fix**: Use explicit array `[2, 3, 3, 4, 4, 5, 5, 6, 6, 7][learnedLevel - 1]`.

### M10. Fire Pillar splash radius scales with level
**Current**: Radius 50 (1x1) at all levels.
**Canonical**: Splash 1 (3x3 = radius 75) at Lv1-5, Splash 2 (5x5 = radius 125) at Lv6-10.
**Fix**: Change the ground effect radius based on level:
```js
radius: learnedLevel >= 6 ? 125 : 75,
```

---

## LOW PRIORITY / DEFERRED

### L1. Sight Rasher accepts Ruwach as Sight substitute (line 11207)
Non-canonical but harmless. Fix when class-restricted buff checks exist.

### L2. Water Ball — no water terrain check + max hits discrepancy
Current [1,4,9,9,25]. Canonical rAthena grid search gives [1,9,25,49,81] maximum.
Deferred until water terrain/cell system is implemented.

### L3. Ice Wall — simplified circular ground effect
No 5-cell line, no per-cell HP, no movement blocking. Needs pathfinding/collision system.

### L4. Heaven's Drive — doesn't reveal hidden enemies
Ground-AoE earth skill should hit and reveal hidden targets. Defer until hidden system expanded.

### L5. Water Ball Lv6-10 (obtainable via Rogue Intimidate)
Not a priority — levels 6-10 are from specific monster copy only. Defer.

---

## Per-Skill Canonical Values vs Current Code

### 800 — Jupitel Thunder ✅ CORRECT
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 20+i*3 = [20-47] | [20-47] | ✅ |
| Cast Time | 2500+i*500 = [2500-7000] | [2500-7000] | ✅ |
| ACD | 0 | 0 | ✅ |
| Hits | 3+i = [3-12] | [3-12] | ✅ |
| MATK% | 100/hit | 100/hit | ✅ |
| Knockback | floor((n+1)/2) | [2,3,3,4,4,5,5,6,6,7] | ✅ |

### 801 — Lord of Vermilion ⚠️ DATA FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 60+i*4 = [60-96] | [60-96] | ✅ |
| Cast Time | 15000-i*500 | 15000-i*500 | ✅ |
| ACD | 0 (cooldown=5000) | 5000 (cooldown=0) | ❌ D12 |
| Per-wave MATK% | 100+i*20 = [100-280] | [100-280] | ✅ |
| Waves | 4 | 4 | ✅ |
| Blind % | 4*Lv | 4*Lv | ✅ |
| Blind applies | NEVER | always | ❌ C2 |

### 802 — Meteor Storm ⚠️ DATA + HANDLER FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | explicit array | same | ✅ |
| Cast Time | 15000 | 15000 | ✅ |
| ACD | 2500+i*500 | [2000,3000,3000,...] | ❌ D13 |
| MATK%/hit | 125 | 125 | ✅ |
| Meteors | floor((lv+2)/2) = [1-6] | [2,3,3,4,4,5,5,6,6,7] | ❌ M9 |
| Hits/meteor | floor((lv+1)/2) | [1,1,2,2,3,3,4,4,5,5] | ✅ |
| Splash/meteor | 75 UE (3x3) | 150 UE (7x7) | ❌ H6 |
| Stun % | 3*Lv | 3*Lv | ✅ |
| Stun applies | NEVER | always | ❌ C3 |

### 803 — Storm Gust ⚠️ MULTIPLE FIXES
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 78 | 78 | ✅ |
| Cast Time | 6000+i*1000 | 6000+i*1000 | ✅ |
| ACD | 0 (cooldown=5000) | 5000 (cooldown=0) | ❌ D12 |
| Per-tick MATK% | 140+i*40 = [140-500] | [140-500] | ✅ |
| Ticks | 10 | 10 | ✅ |
| AoE radius | 175 (7x7) | 225 (9x9) | ❌ H2 |
| Freeze mechanic | every 3rd hit | every 3rd hit | ✅ |
| Freeze applies | NEVER | 150% base | ❌ C1 |
| Frozen immunity | no check | frozen = skip | ❌ H1 |
| Knockback | none | 2 cells/tick | ❌ M8 |

### 804 — Earth Spike ⚠️ DATA FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 12+i*2 = [12-20] | [12-20] | ✅ |
| Cast Time | 700*(i+1) = [700-3500] | 1000*(i+1) = [1000-5000] | ❌ D1 |
| ACD | 0 | 700 | ❌ D2 |
| Hits | i+1 = [1-5] | [1-5] | ✅ |
| MATK% | 100/hit | 100/hit | ✅ |

### 805 — Heaven's Drive ⚠️ DATA + HANDLER FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 28+i*4 = [28-44] | [28-44] | ✅ |
| Cast Time | 1000*(i+1) = [1000-5000] | [1000-5000] | ✅ |
| ACD | 0 | 700 | ❌ D3 |
| Hits | i+1 = [1-5] | [1-5] | ✅ |
| MATK%/hit | 125 (handler) | 100 | ❌ H5 |

### 806 — Quagmire ⚠️ MULTIPLE FIXES
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 5+i*5 = [5-25] | [5-25] | ✅ |
| Cast Time | 0 | 0 | ✅ |
| ACD | 0 | 1000 | ❌ D4 |
| AGI/DEX reduction | 5*(i+1) = [5-25] | 10*(i+1) = [10-50] | ❌ D5/H3 |
| Duration | 5000*(i+1) = [5k-25k] | [5k-25k] | ✅ |
| Boss immune | yes (statusImmune) | NO — works on bosses | ❌ H4 |
| Strip list | 3 buffs | 5+ buffs | ❌ M5 |

### 807 — Water Ball ✅ (simplified)
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | [15,20,20,25,25] | [15,20,20,25,25] | ✅ |
| Cast Time | 1000*(i+1) | [1000-5000] | ✅ |
| MATK%/hit | 130+i*30 = [130-250] | [130-250] | ✅ |
| Max hits | [1,4,9,9,25] | [1,9,25,49,81] (water grid) | ⚠️ L2 |

### 808 — Ice Wall ⚠️ DATA FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 20 | 20 | ✅ |
| Cast Time | 0 | 0 | ✅ |
| HP (effectValue) | 400+i*200 = [400-2200] | [400-2200] | ✅ |
| Duration | 5000*(i+1)+5000 = [10k-55k] | 5000*(i+1) = [5k-50k] | ❌ D6 |

### 809 — Sight Rasher ⚠️ DATA FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 35+i*2 = [35-53] | [35-53] | ✅ |
| Cast Time | 500 | 500 | ✅ |
| ACD | 0 (cooldown=2000) | 2000 (cooldown=0) | ❌ D7 |
| MATK% | 120+i*20 = [120-300] | [120-300] | ✅ |
| Knockback | 5 cells | 5 cells | ✅ |
| Requires Sight | yes | yes | ✅ |

### 810 — Fire Pillar ⚠️ DATA + HANDLER FIX
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 75 | 75 | ✅ |
| Cast Time | 3000-i*300 = [3000-300] | [3000-300] | ✅ |
| ACD | 0 | 1000 | ❌ D8 |
| Hits | i+3 = [3-12] | [3-12] | ✅ |
| Damage | 50+MATK/5, ignores MDEF | same | ✅ |
| Splash | 50 UE (1x1) all levels | 75 Lv1-5, 125 Lv6-10 | ❌ M10 |

### 811 — Frost Nova ⚠️ MULTIPLE FIXES
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| SP | 45-i*2 = [45-27] | [45-27] | ✅ |
| Cast Time | [6000,5600,5200,...] | [6000,6000,5500,5500,...] | ❌ D9 |
| ACD | 0 | 1000 | ❌ D11 |
| MATK% | 73+i*7 = [73-136] | [73,80,87,93,...,133] | ❌ D10 |
| Freeze chance | 33+5*lv = [38-83] | [38-83] | ✅ |
| Freeze applies | NEVER | per chance | ❌ C4 |
| Freeze duration | 12000 fixed | 1500*lv = [1500-15000] | ❌ M6 |

### 812 — Sense ⚠️ MINOR
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| All data | correct | correct | ✅ |
| Element level | missing | should include | ❌ M7 |

### 813 — Sight Blaster ⚠️ CRITICAL
| Field | Current | Canonical | Status |
|-------|---------|-----------|--------|
| All data | correct | correct | ✅ |
| Reactive trigger | MISSING | auto-fire on approach | ❌ C5 |

---

## Implementation Phases

### Phase 1: Data Fixes (ro_skill_data_2nd.js) — 13 corrections
**Effort**: ~20 min | **Risk**: Low (no logic changes)

```js
// D1: Earth Spike cast time
castTime: 1000*(i+1)    // was 700*(i+1)

// D2: Earth Spike ACD
afterCastDelay: 700      // was 0

// D3: Heaven's Drive ACD
afterCastDelay: 700      // was 0

// D4: Quagmire ACD
afterCastDelay: 1000     // was 0

// D5: Quagmire effectValue
effectValue: 10*(i+1)    // was 5*(i+1)

// D6: Ice Wall duration
duration: 5000*(i+1)     // was 5000*(i+1)+5000

// D7: Sight Rasher ACD/cooldown swap
afterCastDelay: 2000, cooldown: 0   // was afterCastDelay: 0, cooldown: 2000

// D8: Fire Pillar ACD
afterCastDelay: 1000     // was 0

// D9: Frost Nova cast time
castTime: [6000,6000,5500,5500,5000,5000,4500,4500,4000,4000][i]

// D10: Frost Nova effectValue
effectValue: [73,80,87,93,100,107,113,120,127,133][i]

// D11: Frost Nova ACD
afterCastDelay: 1000     // was 0

// D12: LoV + Storm Gust ACD/cooldown swap
afterCastDelay: 5000, cooldown: 0   // was afterCastDelay: 0, cooldown: 5000

// D13: Meteor Storm ACD
afterCastDelay: [2000,3000,3000,4000,4000,5000,5000,6000,6000,7000][i]
```

### Phase 2: Critical Status Effect Fixes (C1-C4) — 4 one-line fixes
**Effort**: ~10 min | **Risk**: Low

Fix the `applyStatusEffect` call signature on lines 19663, 19717, 19762, 11303.

### Phase 3: High Priority Handler Fixes (H1-H6)
**Effort**: ~45 min | **Risk**: Medium

1. **H1**: Storm Gust frozen immunity check (add `if frozen → continue` in tick loop)
2. **H2**: Storm Gust AoE radius 175 → 225 (two locations)
3. **H3**: Quagmire reduction values (handler reads from effect data, auto-fixed by D5)
4. **H4**: Quagmire remove boss immunity check
5. **H5**: Heaven's Drive 125% → 100% MATK
6. **H6**: Meteor Storm splash 75 → 150

### Phase 4: Medium Handler Fixes (M1-M10)
**Effort**: ~60 min | **Risk**: Medium

1. **M1**: Add Lex Aeterna to Storm Gust/LoV/Meteor Storm ticks
2. **M2**: Add Lex Aeterna to Sight Rasher/Frost Nova
3. **M3**: Fire Pillar auto-remove oldest (not error)
4. **M4**: Fire Pillar add checkDamageBreakStatuses
5. **M5**: Quagmire strip list: add `'adrenaline_rush'`, `'improve_concentration'`
6. **M6**: Frost Nova freeze duration: `1500 * learnedLevel`
7. **M7**: Sense add elementLevel
8. **M8**: Storm Gust 2-cell knockback per tick
9. **M9**: Meteor Storm meteor count array
10. **M10**: Fire Pillar splash radius scaling

### Phase 5: Sight Blaster Implementation (C5)
**Effort**: ~30 min | **Risk**: Medium

Add proximity trigger tick to server loop.

---

## Testing Checklist

### Status Effects
- [ ] Storm Gust: freeze occurs on 3rd hit (150% base chance)
- [ ] Storm Gust: frozen enemies stop taking further SG ticks
- [ ] Storm Gust: boss/undead take all 10 ticks (no freeze = max damage)
- [ ] Lord of Vermilion: blind at 4*Lv% per wave
- [ ] Meteor Storm: stun at 3*Lv% per hit
- [ ] Frost Nova: freeze at 38-83%, duration scales 1.5s-15s per level

### AoE / Damage
- [ ] Storm Gust: 9x9 area (radius 225), not 7x7
- [ ] Storm Gust: 2-cell knockback on non-frozen hits
- [ ] Meteor Storm: correct meteor count [2,3,3,4,4,5,5,6,6,7]
- [ ] Meteor Storm: each meteor splashes 7x7 (radius 150)
- [ ] Heaven's Drive: 100% MATK per hit, not 125%
- [ ] Quagmire: -10/-20/-30/-40/-50 AGI/DEX (not -5 to -25)
- [ ] Quagmire: works on bosses
- [ ] Fire Pillar: splash 3x3 at Lv1-5, 5x5 at Lv6-10

### Data Values
- [ ] Earth Spike: cast time [1000,2000,3000,4000,5000], ACD 700ms
- [ ] Heaven's Drive: ACD 700ms
- [ ] Ice Wall: duration [5s,10s,...,50s] (not [10s,...,55s])
- [ ] Frost Nova: cast [6000,6000,5500,5500,...,4000,4000], ACD 1000ms
- [ ] Frost Nova: MATK% [73,80,87,93,100,107,113,120,127,133]
- [ ] LoV/Storm Gust: ACD 5000ms (not cooldown 5000ms)
- [ ] Meteor Storm: ACD [2000,3000,3000,...,7000]
- [ ] Sight Rasher: ACD 2000ms (not cooldown)
- [ ] Fire Pillar: ACD 1000ms

### Special Mechanics
- [ ] Sight Blaster: buff icon appears, auto-fires on enemy approach, 100% MATK fire + 3-cell KB
- [ ] Sight Blaster: consumed after trigger (single-use)
- [ ] Lex Aeterna consumed by ground AoE first tick
- [ ] Fire Pillar: 6th pillar auto-removes oldest
- [ ] Quagmire: strips Adrenaline Rush + Improve Concentration

---

## Session 2 Fixes (2026-03-20) — Wizard Skill Behavior & Timing Corrections

**Scope:** 9 server fixes + 2 client changes. Verified against rAthena source (`map_foreachinallrange` for Sight Blaster, per-cell pathfinding for Ice Wall).

### Server Fixes (index.js)

#### F1. Sight Blaster — Multi-Target Hit Before Consuming Buff
**Bug:** `break` statement after hitting the first enemy caused Sight Blaster to only damage one target per trigger.
**Fix:** Removed `break`; buff now hits ALL enemies in 3x3 range simultaneously, then consumes the buff after the loop completes.
**Source:** rAthena `map_foreachinallrange` processes all entities in range before setting `val2=0` to end the buff.

#### F2. Ice Wall — Movement Blocking for Enemies AND Players
**Bug:** Ice Wall had no movement blocking — enemies and players could walk through it.
**Fix:** Per-cell-step knockback collision check: enemies stop at the wall edge when knocked back instead of passing through. Enemies AND players are blocked from walking through ice wall cells. HP decay tick: 50 HP/sec via time-based calculation in ground effect loop.
**Note:** This resolves the L3 "deferred" limitation from the original audit — Ice Wall now blocks movement.

#### F3. Storm Gust — While-Loop Catch-Up Timing Pattern
**Bug:** Timing drift could cause missed ticks under server load.
**Fix:** While-loop catch-up pattern with interval-advance (`lastTickTime += interval` instead of `lastTickTime = now`). Ground effect tick reduced from 500ms to 250ms for more responsive wave processing.

#### F4. Lord of Vermilion — While-Loop Catch-Up Timing Pattern
**Bug:** Same timing drift issue as Storm Gust.
**Fix:** Same while-loop catch-up pattern for consistency with Storm Gust.

#### F5. Meteor Storm — Meteor Count Array + Wave Interval + Duration
**Bug:** Meteor count array `[2,3,3,4,4,5,5,6,6,7]` was wrong (this was the array from M9 in the original audit — it was applied but then reverted or reintroduced incorrectly).
**Fix:** Corrected to `[2,2,3,3,4,4,5,5,6,6]`. Reduced wave interval from 400ms to 300ms. Fixed duration calculation for proper meteor timing.

#### F6. Heaven's Drive — MATK% Corrected to 125%
**Bug:** MATK% was set to 100% per hit.
**Fix:** Corrected to 125% per hit (rAthena pre-renewal verified). Added diagnostic logging for debugging.
**Note:** This is consistent with H5 being REVERTED in the original audit — 125% is confirmed correct.

#### F7. Frost Nova — Client-Side Self-Centered AoE Routing (see Client section)
**Bug:** Server handler was already correct (uses caster position). The fix was client-side only.
**Fix:** SkillTreeSubsystem now routes self-centered AoE skills correctly (see Client Fixes below).

#### F8. Quagmire — Three-Layer Speed Reduction
**Bug:** Quagmire movement speed debuff was incomplete — only partial application.
**Fix:** Three-layer speed reduction system:
1. **Inline in enemyMoveToward**: Applies buff + halves movement on first contact tick
2. **Ground effect tick**: Refreshes buff every 500ms while in zone
3. **getCombinedModifiers**: Reads `moveSpeedBonus: -50` for future tick speed calculation
Also added player movement blocking via `player:position` handler.

#### F9. knockbackTarget — Per-Cell Ice Wall Collision
**Bug:** Knockback passed through Ice Wall cells. Also had a `ReferenceError` crash on `finalX`/`finalY` variables.
**Fix:** Per-cell-step collision check against all active ice_wall ground effects. Uses `finalX`/`finalY` instead of `newX`/`newY` for correct variable references. Entity stops at the wall edge cell instead of passing through.

### Client Fixes

#### CF1. SSenseResultPopup — New Draggable Monster Info Panel
**Files:** `UI/SSenseResultPopup.h`, `UI/SSenseResultPopup.cpp` (NEW)
- `FSenseResultData` struct for all monster info fields
- Draggable non-blocking popup with RO Classic brown/gold theme
- Title bar drag, X close button only (no auto-dismiss)
- Shows: Name, Level, HP, Element+Level, Race, Size, DEF/MDEF, Base EXP/Job EXP

#### CF2. EnemySubsystem — Sense Result Handler
**Files:** `UI/EnemySubsystem.h`, `UI/EnemySubsystem.cpp`
- Added `skill:sense_result` socket handler
- `ShowSensePopup()` / `HideSensePopup()` methods
- AlignmentWrapper with `SelfHitTestInvisible` for proper input passthrough

#### CF3. SkillTreeSubsystem — Self-Centered AoE Routing Fix
**File:** `UI/SkillTreeSubsystem.cpp`
- Fixed self-centered AoE routing: skills with `targetType=='aoe'` AND `range<=0` now fire immediately via `UseSkillOnTarget` instead of entering ground targeting mode
- Fixes: Frost Nova (811), Sight Rasher (809), Magnum Break (102)
