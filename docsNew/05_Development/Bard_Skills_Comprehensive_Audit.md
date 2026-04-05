# Bard Skills Comprehensive Audit & Fix Plan (v3 — ALL IMPLEMENTED)

**Date:** 2026-03-17
**Scope:** All 20 Bard skills (IDs 1500-1537) — 12 solo + 8 ensemble
**Status:** ALL 22 BUGS FIXED + 1 ADDITIONAL rAthena-VERIFIED FIX
**Sources:** iRO Wiki Classic, RateMyServer, rAthena source (status.cpp, skill.cpp, battle.cpp, skill_db.yml, pre-re/skill_db.yml), divine-pride, GameFAQs Bard guides (Jath, aldoteng, Dikiwinky), Project Alfheim Wiki, idRO Klasik Wiki

### v3 Additional Fixes (2026-03-17)
- **Bug #19 IMPLEMENTED**: Song overlap → Dissonance. Two same-type performances overlapping now converts the overlap zone to Dissonance Lv1 MISC damage (song+song) instead of applying song buffs. Song+Dance overlap is fine (coexist).
- **NEW FIX: Frost Joker/Scream blocked during performance.** rAthena `AllowWhenPerforming` flag confirmed: ONLY Adaptation, Musical Strike, and Slinging Arrow are allowed. Frost Joker and Scream do NOT have this flag. Fixed.

---

## Audit Summary

| Category | Count | Severity |
|----------|-------|----------|
| Critical Bugs (breaks intended behavior) | 5 | HIGH |
| Important Data/Logic Errors | 10 | MEDIUM |
| Minor Gaps (blocked by missing systems or edge cases) | 6 | LOW |
| **Total Issues** | **21** | |
| Aspects Verified Correct | See "Verified Correct" section | |

**v2 Changes from v1:** Deep research corrected 7 items previously marked "CORRECT" — Musical Strike damage formula, A Whistle duration, Pang Voice success rate, Unbarring Octave canonicity, caster buff exclusion, Silence CC behavior, performance movement speed formula. Added 7 net new issues.

---

## Skills Verified CORRECT (No Changes Needed)

### Music Lessons (1500) — CORRECT
- [x] +3 ATK/level with instruments (`getPassiveSkillBonuses()` line 652-655)
- [x] Scales all 4 song formulas (Whistle, Assassin Cross, Bragi, Apple of Idun)
- [x] Scales Dissonance damage (`lessonsLv * 3`)

### Adaptation (1503) — CORRECT
- [x] SP cost: 1, ACD: 300ms
- [x] Cancels active performance
- [x] Cannot cancel in first 5 seconds (line 13938-13942)

### Encore (1504) — MOSTLY CORRECT (see Bug #14)
- [x] SP cost: 1 + ceil(lastSongSP / 2)
- [x] ACD: 300ms, Cooldown: 10s
- [x] Validates: not currently performing, has previous performance, enough SP
- [x] Weapon check (instrument required)
- [x] Replays at stored level, not current learned level

### Dissonance (1505) — MOSTLY CORRECT (see Bug #10)
- [x] SP cost: `18+i*3` = 18-30 matches `15+SkillLv*3`
- [x] Duration: 30000ms (30 seconds)
- [x] Damage formula: `30 + 10*SkillLv + MusicLessonsLv*3` — correct
- [x] Damage fires every 3 seconds (tick counter % 3)
- [x] SP drain: 1 SP / 3 seconds

### Frost Joker (1506) — MOSTLY CORRECT (see Bugs #11, #12)
- [x] SP cost: `12+i*2` = 12-20 matches `10+2*SkillLv`
- [x] Freeze chance: `20+i*5` = 20-40% matches `(15+5*SkillLv)%`
- [x] Screen-wide (750 UE radius)
- [x] Affects ALL enemies AND all players including self/party
- [x] Boss immunity check
- [x] NOT a performance — allowed during active performance

### A Whistle (1507) — MOSTLY CORRECT (see Bug #6)
- [x] SP cost: `24+i*4` = 24-60 matches `20+SkillLv*4`
- [x] FLEE formula: `SkillLv + floor(AGI/10) + floor(ML*0.5)` — correct
- [x] PD formula: `floor((SkillLv+1)/2) + floor(LUK/30) + floor(ML*0.2)` — correct
- [x] SP drain: 1 SP / 5 seconds
- [x] Buff modifiers in getCombinedModifiers() correct

### Assassin Cross of Sunset (1502) — MOSTLY CORRECT (see Bugs #4, #13)
- [x] SP cost: `38+i*3` = 38-65 matches `35+SkillLv*3`
- [x] Duration: 120000ms (120 seconds)
- [x] ASPD formula: `10 + SkillLv + floor(AGI/20) + floor(ML/2)` — correct
- [x] SP drain: 1 SP / 3 seconds

### A Poem of Bragi (1501) — CORRECT
- [x] SP cost: `40+i*5` = 40-85 matches `35+SkillLv*5`
- [x] Duration: 180000ms (180 seconds)
- [x] VCT formula: `SkillLv*3 + floor(DEX/10) + ML` — correct
- [x] ACD formula: `(SkillLv<10 ? SkillLv*3 : 50) + floor(INT/5) + ML*2` — correct
- [x] Cast time + ACD reduction hooks both wired

### Apple of Idun (1508) — CORRECT
- [x] SP cost: `40+i*5` = 40-85 matches `35+SkillLv*5`
- [x] Duration: 180000ms (180 seconds)
- [x] MaxHP formula: `5 + SkillLv*2 + floor(VIT/10) + floor(ML/2)` — correct
- [x] HP/tick formula: `(30+SkillLv*5) + floor(VIT/2) + ML*5` — correct
- [x] HP regen fires every 6 seconds

### Performance System Infrastructure — MOSTLY CORRECT (see multiple bugs)
- [x] `PERFORMANCE_CONSTANTS.SOLO_AOE_RADIUS`: 175 (7x7 cells)
- [x] `PERFORMANCE_CONSTANTS.EFFECT_LINGER_DURATION`: 20000 (20s buff linger)
- [x] `SONG_SP_DRAIN` mapping all correct
- [x] Ground effect follows caster position (correct for pre-renewal solo songs)
- [x] Death auto-cancel, SP depletion auto-cancel, duration expiry auto-cancel
- [x] Disconnect/skill reset cleanup

---

## CRITICAL BUGS (Must Fix)

### Bug #1: Starting New Performance While Performing Should Cancel Old One
**File:** `server/src/index.js` line 13900-13904
**Current:** Returns error "Already performing. Cancel first with Adaptation."
**Canonical:** Starting a new song auto-cancels the old one. In rAthena, casting a new song while `SC_DANCING` is active triggers `status_change_end(bl, SC_DANCING)` before the new song starts. This is the basis of "song flashing" technique documented on iRO Wiki.
**Sources:** iRO Wiki Bard, GameFAQs aldoteng guide, rAthena skill.cpp
**Fix:** Replace error block with `cancelPerformance(characterId, player, 'new_performance')` before starting.

### Bug #2: "Unbarring Octave" (1510) Is NOT a Canonical Pre-Renewal Bard Skill
**File:** `server/src/ro_skill_data_2nd.js` line 113, `server/src/index.js`
**Current:** ID 1510 `unbarring_octave` exists as a separate performance song that "prevents skill use in AoE."
**Canonical:** "Unbarring Octave" is iRO's localized name for **Frost Joker** (BA_FROSTJOKER, ID 318). It is the screen-wide freeze skill, NOT a skill-blocking song. The skill-blocking effect belongs exclusively to **Loki's Veil** (BD_ROKISWEIL, ID 311), which is an ensemble skill requiring both Bard AND Dancer.
**Sources:** iRO Wiki (Unbarring Octave redirects to Frost Joker), RateMyServer skill DB 318, divine-pride skill 318
**Impact:** The project has a non-canonical skill occupying an ID slot. The "skill block in AoE" effect does not exist as a solo Bard performance in pre-renewal RO.
**Fix:** Remove ID 1510 `unbarring_octave` from skill data and remove from `SONG_SP_DRAIN`, `PERFORMANCE_BUFF_MAP`, and `OFFENSIVE_PERFORMANCES`. The skill-blocking AoE effect should only be implemented when the ensemble system (Loki's Veil) is built.

### Bug #3: Musical Strike Damage Formula Is Wrong
**File:** `server/src/ro_skill_data_2nd.js` line 114
**Current:** `effectValue: 150+i*25` → 150/175/200/225/250%
**Canonical:** `[60 + (SkillLv * 40)]%` → 100/140/180/220/260%
**Sources:** iRO Wiki Melody Strike, RateMyServer skill 316, rAthena pre-renewal skill_db
**Impact:** Damage is wrong at every level. Lv1 is 50% too high (150 vs 100), Lv5 is close but still wrong (250 vs 260).
**Fix:** Change to `effectValue: 60+((i+1)*40)` → 100/140/180/220/260

### Bug #4: ASPD Buff Stacking — Assassin Cross Stacks With Adrenaline Rush/Quicken
**File:** `server/src/ro_buff_system.js` lines 832-835, 873-876
**Current:** `song_assassin_cross`, `adrenaline_rush`, `two_hand_quicken`, `spear_quicken` all independently multiply `aspdMultiplier`.
**Canonical:** These are in rAthena's "Haste2" exclusion group — only the strongest applies. They DO stack with ASPD potions (Haste1 group). Cross-group stacking is allowed, within-group is not.
**Sources:** rAthena status.cpp SC_ASSNCROS, iRO Wiki Classic Impressive Riff, idRO Klasik Wiki, rAthena issue #9041
**Fix:** In `getCombinedModifiers()`, track Haste2 group separately. Only apply the single highest ASPD bonus from the group.

### Bug #5: Song Caster Is Incorrectly Included in Buff Effects
**File:** `server/src/index.js` lines 19284-19327
**Current:** Supportive performance buff loop iterates ALL `connectedPlayers.entries()` in AoE, including the performer themselves.
**Canonical:** Solo support songs affect ALL players in AoE **EXCEPT the caster**. The performing Bard/Dancer does NOT receive their own song buff. Confirmed by iRO Wiki, MyRoLife guide, multiple player guides.
**Sources:** iRO Wiki Bard, iRO Wiki Ensemble Skill, MyRoLife Bard/Dancer guide
**Impact:** Bards currently buff themselves with Bragi (faster casts), Whistle (more FLEE), etc. This makes solo Bards significantly more powerful than intended.
**Fix:** Add caster exclusion to the supportive performance buff loop:
```js
if (pid === charId) continue; // Caster does not receive own song buff
```

---

## IMPORTANT BUGS (Should Fix)

### Bug #6: A Whistle Duration Is Wrong
**File:** `server/src/ro_skill_data_2nd.js` line 110
**Current:** `duration: 60000` (60 seconds)
**Canonical:** 180000ms (180 seconds) — same as Bragi and Apple of Idun. The "60 second" value likely confused with ensemble duration (all ensembles are 60s).
**Sources:** iRO Wiki Perfect Tablature, RateMyServer skill 319
**Fix:** Change `duration: 60000` → `duration: 180000`

### Bug #7: Performance Movement Speed Formula Wrong
**File:** `server/src/index.js` — missing entirely
**Current:** No movement restriction during performance (full speed).
**Canonical:** Performance reduces movement to 25% base (1/4 speed) WITHOUT Music/Dance Lessons. With Music Lessons Lv N: `(25 + 2.5*N)%` of normal speed. Max 50% at Lv10. Ensembles: 0% (cannot move at all).
**Sources:** iRO Wiki Music Lessons, iRO Wiki Dance Lessons, GameFAQs aldoteng guide, MyRoLife guide
**Note:** Previous audit v1 said "0% without ML" — corrected to 25% base per deep research.
**Fix:** Apply movement multiplier when performance starts. Formula: `performanceSpeedPercent = 25 + 2.5 * lessonsLv`

### Bug #8: All 8 Ensemble Skill Prerequisites Are Wrong
**File:** `server/src/ro_skill_data_2nd.js` lines 116-123
**Current:** All point to `Music Lessons (1500)` at varying levels.
**Canonical (confirmed by multiple sources):**

| ID | Skill | Current Prereq | Correct Prereq (Bard side) |
|----|-------|---------------|----------------------------|
| 1530 | Lullaby | Music Lessons 10 | **A Whistle (1507) Lv 10** |
| 1531 | Mr. Kim A Rich Man | Music Lessons 5 | **Invulnerable Siegfried (1537) Lv 3** |
| 1532 | Eternal Chaos | Music Lessons 5 | **Loki's Veil (1535) Lv 1** |
| 1533 | Drum on Battlefield | Music Lessons 5 | **Apple of Idun (1508) Lv 10** |
| 1534 | Ring of Nibelungen | Music Lessons 10 | **Drum on Battlefield (1533) Lv 3** |
| 1535 | Loki's Veil | Music Lessons 10 | **Assassin Cross (1502) Lv 10** |
| 1536 | Into the Abyss | Music Lessons 5 | **Lullaby (1530) Lv 1** |
| 1537 | Invulnerable Siegfried | Music Lessons 10 | **Poem of Bragi (1501) Lv 10** |

**Sources:** iRO Wiki Ensemble Skills, RateMyServer Bard skill tree, GameFAQs Dikiwinky ensemble guide

### Bug #9: Weapon Swap Does Not Cancel Performance
**File:** `server/src/index.js` — equipment change handler
**Current:** `cancelPerformance()` documents `'weapon_swap'` as a reason but is never called.
**Canonical:** Switching weapons instantly cancels performance. This is the primary cancel method in competitive play ("dagger-stopping tactic"). Bypasses Adaptation's 5-second window restriction.
**Sources:** iRO Wiki Bard, MyRoLife guide ("carry a Knife"), GameFAQs aldoteng guide
**Fix:** In equipment change handler, check if performing and call `cancelPerformance(charId, player, 'weapon_swap')`.

### Bug #10: Dissonance Damage Uses MDEF Reduction (Should Be MISC/Fixed)
**File:** `server/src/index.js` lines 19248-19249
**Current:** `mdefReduction = Math.max(0, 1 - hardMdef / 100); finalDmg = Math.floor(dmg * mdefReduction);`
**Canonical:** Dissonance is MISC damage type (BF_MISC). It ignores BOTH DEF and MDEF entirely. Fixed formula only.
**Sources:** rAthena battle.cpp BF_MISC for BA_DISSONANCE, iRO Wiki Unchained Serenade
**Fix:** `const finalDmg = Math.max(1, dmg);` — no defense reduction at all.

### Bug #11: Frost Joker Freeze Duration Too Short
**File:** `server/src/index.js` line 14015, 14025
**Current:** Hardcoded 5000ms (5 seconds) for all targets.
**Canonical:** Base freeze duration is ~12 seconds. Reduced by target's equipment MDEF: `Duration = 12000 - 12000 * itemMDEF / 100`. Minimum ~3 seconds.
**Sources:** iRO Wiki Unbarring Octave, RateMyServer status resistance guide
**Fix:** Use proper freeze duration calculation instead of hardcoded 5000ms.

### Bug #12: Frost Joker Party Members Should Have Reduced Freeze Chance
**File:** `server/src/index.js` lines 14021-14028
**Current:** Same freeze chance applied to all players including party members.
**Canonical:** Party members have approximately half the base freeze chance.
**Sources:** iRO Wiki Unbarring Octave ("low probability" for party), GameFAQs Jath guide
**Fix:** When iterating players, check if target is in same party. If so, apply `freezeChance / 2`.

### Bug #13: Assassin Cross of Sunset Should Not Affect Bow Users
**File:** `server/src/index.js` performance tick (buff application loop)
**Current:** ASPD buff applied to all players in AoE regardless of weapon type.
**Canonical:** Assassin Cross of Sunset's ASPD boost does NOT apply to characters using bows.
**Sources:** iRO Wiki Classic Impressive Riff, idRO Klasik Wiki
**Fix:** In buff application loop, check target's weapon type. Skip if `weaponType === 'bow'`.

### Bug #14: Encore Should Clear Remembered Skill After Use
**File:** `server/src/index.js` lines 13954-13998
**Current:** `lastPerformanceSkillId` persists after Encore is used. Player can theoretically chain Encore → cancel → Encore → cancel indefinitely at half SP.
**Canonical:** Using Encore clears the remembered skill. You cannot chain Encore repeatedly. Must perform a full song to re-set the Encore memory.
**Sources:** iRO Wiki Encore, GameFAQs aldoteng guide
**Fix:** After `startPerformance()` in the Encore handler, set `player.lastPerformanceSkillId = null`.

### Bug #15: Pang Voice Has Formula-Based Success Rate (Not 100%)
**File:** `server/src/index.js` line 14079
**Current:** `applyStatusEffect(player, enemy, 'confusion', 100, 15000)` — 100% chance
**Canonical:** Success rate depends on caster base level vs target stats. Estimated formula: `50 + (casterBaseLv - targetBaseLv) - targetVIT/5 - targetLUK/5`%. Not a flat 100%.
**Sources:** iRO Wiki Pang Voice, RateMyServer skill 1010
**Fix:** Calculate success rate based on level/stat comparison. Replace `100` with calculated chance.

---

## MINOR ISSUES (Low Priority / Blocked by Missing Systems)

### Bug #16: Silence CC Should NOT Cancel Solo Performances
**File:** `server/src/index.js` line 19176
**Current:** `ccMods.isSilenced` is in the CC cancel check, canceling solo performances on silence.
**Canonical:** Silence does NOT cancel solo songs (the song is already active — silence prevents NEW casts but doesn't stop an ongoing performance). Silence only cancels ensemble performances.
**Sources:** rAthena status.cpp, MyRoLife guide ("Silence has no effect on ongoing songs")
**Fix:** Remove `ccMods.isSilenced` from the solo performance CC check. Only apply for ensembles (deferred).

### Bug #17: Dispel Should Cancel Performance (Not Implemented)
**Current:** No Dispel hook for performance cancellation.
**Canonical:** Sage's Dispel can cancel a Bard/Dancer's active performance.
**Sources:** MyRoLife guide, multiple player guides
**Fix:** In the Dispel skill handler, check if target is performing and call `cancelPerformance()`.

### Bug #18: Heavy Damage (>25% MaxHP) Should Cancel Performance
**Current:** No damage threshold check for performance cancellation.
**Canonical:** Taking more than 25% of MaxHP in a single hit cancels the performance.
**Sources:** MyRoLife guide, Project Alfheim Wiki
**Fix:** In damage application pipeline, check if target is performing and damage > maxHealth * 0.25.

### Bug #19: Song AoE Overlap Creates Dissonance (Not Implemented)
**Current:** Multiple Bard song AoEs can coexist without interaction.
**Canonical:** When two Bard song AoEs overlap, the intersection zone produces Dissonance Lv1 damage instead of stacking buffs.
**Sources:** iRO Wiki Bard, multiple player guides
**Impact:** Complex mechanic, low priority without multiple Bards.

### Bug #20: Invulnerable Siegfried Element Resist Values Wrong
**File:** `server/src/ro_skill_data_2nd.js` line 123
**Current:** `effectValue: 10+i*10` → 10/20/30/40/50 (appears to be status resist only)
**Canonical:** Element resist: 60/65/70/75/80%. Status resist: 10/20/30/40/50%. Need two separate values.
**Sources:** iRO Wiki Acoustic Rhythm, RateMyServer skill 313
**Fix:** Store both values. Element resist = `55 + 5*Lv`, status resist = `10*Lv`.

### Bug #21: Loki's Veil SP Cost Wrong + Other Ensemble Data
**File:** `server/src/ro_skill_data_2nd.js` line 121
**Loki's Veil:** Current `spCost: 30` → canonical `15`
**Inv. Siegfried SP:** Current `20+i*5` → canonical `15+5*Lv` = 20/25/30/35/40 (matches, OK)
**Mr. Kim SP:** Current `20+i*4` → canonical `16+4*Lv` = 20/24/28/32/36 (matches, OK)

---

## Adaptation "Last 5 Seconds" Rule — CLARIFICATION

**File:** `server/src/index.js` line 13940
**Current:** Cannot cancel in first OR last 5 seconds: `perfElapsed < 5000 || perfRemaining < 5000`
**Canonical:** Research only consistently mentions the "first 5 seconds" restriction. The "last 5 seconds" restriction is not well-documented. Most sources only say "cannot be used within the first 5 seconds." However, this is a minor defensive check and keeping it is harmless.
**Decision:** Keep as-is. The last-5-seconds check prevents edge cases and doesn't harm gameplay.

---

## Fix Execution Plan

### Phase A: Critical Fixes (5 bugs)

**A1: New Performance Auto-Cancels Old** (Bug #1)
- File: `server/src/index.js` line 13900-13904
- Change: Replace error block with `cancelPerformance()` + proceed to start new.

**A2: Remove Non-Canonical Unbarring Octave** (Bug #2)
- File: `server/src/ro_skill_data_2nd.js` line 113
- File: `server/src/index.js` — SONG_SP_DRAIN, PERFORMANCE_BUFF_MAP, OFFENSIVE_PERFORMANCES
- Changes:
  1. Remove ID 1510 `unbarring_octave` from skill data (or mark as `removed: true`)
  2. Remove `'unbarring_octave'` from SONG_SP_DRAIN, PERFORMANCE_BUFF_MAP, OFFENSIVE_PERFORMANCES
  3. The skill-blocking AoE effect belongs to Loki's Veil (ensemble) — defer to ensemble implementation
- Note: If you want to keep this as a custom skill that doesn't exist in canonical RO, rename it and document that it's custom. Otherwise, remove it.

**A3: Fix Musical Strike Damage** (Bug #3)
- File: `server/src/ro_skill_data_2nd.js` line 114
- Change: `effectValue: 150+i*25` → `effectValue: 60+((i+1)*40)` (gives 100/140/180/220/260)

**A4: ASPD Haste2 Exclusion Group** (Bug #4)
- File: `server/src/ro_buff_system.js`
- Change: In `getCombinedModifiers()`, track Haste2 group (`song_assassin_cross`, `adrenaline_rush`, `two_hand_quicken`, `spear_quicken`) separately. Only apply the single highest ASPD bonus from the group.

**A5: Exclude Caster From Own Song Buff** (Bug #5)
- File: `server/src/index.js` line 19285 (supportive buff loop)
- Change: Add `if (pid === charId) continue;` at the top of the ally loop.

### Phase B: Important Fixes (10 bugs)

**B1: Fix A Whistle Duration** (Bug #6)
- File: `server/src/ro_skill_data_2nd.js` line 110
- Change: `duration: 60000` → `duration: 180000`

**B2: Performance Movement Speed Restriction** (Bug #7)
- File: `server/src/index.js`
- Add: Movement speed multiplier when performing: `25 + 2.5 * lessonsLv`% of normal.
- Server-side enforcement: reject or clamp position updates that exceed allowed speed.

**B3: Fix All 8 Ensemble Prerequisites** (Bug #8)
- File: `server/src/ro_skill_data_2nd.js` lines 116-123
- Change all 8 Bard ensemble prerequisites per table above.
- Also fix 8 Dancer ensemble prerequisites (IDs 1550-1557).

**B4: Weapon Swap Cancels Performance** (Bug #9)
- File: `server/src/index.js` — equipment change handler
- Add: Call `cancelPerformance(charId, player, 'weapon_swap')` when weapon type changes during performance.

**B5: Dissonance to MISC Damage** (Bug #10)
- File: `server/src/index.js` line 19248-19249
- Change: Remove MDEF reduction. `const finalDmg = Math.max(1, dmg);`

**B6: Frost Joker Freeze Duration** (Bug #11)
- File: `server/src/index.js` lines 14015, 14025
- Change: `5000` → proper freeze duration calculation: `Math.max(3000, 12000 - Math.floor(12000 * (targetItemMDEF || 0) / 100))`

**B7: Frost Joker Party Member Reduced Chance** (Bug #12)
- File: `server/src/index.js` lines 14021-14028
- Change: For party members (when party system exists) or for self, apply `freezeChance / 2`. For now, apply half chance to self only.

**B8: ACoS Bow User Exclusion** (Bug #13)
- File: `server/src/index.js` performance tick, buff application
- Change: Skip ASPD buff for players with `weaponType === 'bow'`.

**B9: Encore Clears Remembered Skill** (Bug #14)
- File: `server/src/index.js` line ~13997
- Change: After `startPerformance()` in Encore handler, add `player.lastPerformanceSkillId = null; player.lastPerformanceLevel = null;`

**B10: Pang Voice Success Rate Formula** (Bug #15)
- File: `server/src/index.js` line 14079
- Change: Calculate `chance = Math.max(5, Math.min(95, 50 + (player.baseLv || 1) - (enemy.level || 1) - Math.floor((enemy.vit || 0) / 5) - Math.floor((enemy.luk || 0) / 5)))` instead of 100.

### Phase C: Minor Fixes (6 bugs)

**C1: Remove Silence from Solo Performance CC Cancel** (Bug #16)
- File: `server/src/index.js` line 19176
- Change: Remove `ccMods.isSilenced` from the check. Silence does not cancel solo performances.

**C2: Dispel Cancels Performance** (Bug #17)
- Add `cancelPerformance()` call in the Dispel skill handler when target is performing.

**C3: Heavy Damage Cancels Performance** (Bug #18)
- In damage application, check `damage > target.maxHealth * 0.25 && isPerforming(target)` → cancel.

**C4: Song Overlap Dissonance** (Bug #19)
- Defer — complex mechanic, requires tracking all active performance AoEs.

**C5: Fix Invulnerable Siegfried Values** (Bug #20)
- File: `server/src/ro_skill_data_2nd.js` line 123
- Need to store element resist (60-80%) and status resist (10-50%) separately.

**C6: Fix Loki's Veil SP** (Bug #21)
- File: `server/src/ro_skill_data_2nd.js` line 121
- Change: `spCost: 30` → `spCost: 15`. Also Dancer mirror (1555).

---

## Canonical Performance System Rules (Reference)

Compiled from all research sources — this is the definitive pre-renewal behavior:

| Mechanic | Pre-Renewal Canonical |
|----------|----------------------|
| Solo AoE size | 7x7 cells (175 UE radius) |
| Ensemble AoE size | 9x9 cells (225 UE radius) |
| Solo AoE movement | Follows caster |
| Ensemble AoE movement | Fixed between performers |
| Movement speed (solo) | `(25 + 2.5 * LessonsLv)%` of normal (25%-50%) |
| Movement speed (ensemble) | 0% — cannot move |
| Buff linger after leaving AoE | 20 seconds flat |
| Buff linger refresh | Does NOT reset if re-entered before expiry |
| Caster receives own buff? | **NO** — caster excluded |
| Who gets solo song buffs? | ALL players in AoE (not party-only) except caster |
| Who gets ensemble buffs? | Party members only in AoE |
| Starting new song | Auto-cancels previous song |
| CC that cancels | Stun, Freeze, Stone |
| CC that does NOT cancel solo | Silence, Bleeding, Confusion, Sleep |
| Silence cancels ensemble? | Yes (ensembles only) |
| Weapon swap cancels? | Yes — instant (bypasses 5s Adaptation gate) |
| Dispel cancels? | Yes |
| Death cancels? | Yes |
| SP depletion cancels? | Yes |
| 25%+ HP single hit cancels? | Yes |
| Skills usable during perf | Musical Strike, Slinging Arrow, Frost Joker, Scream |
| Encore during performance? | No — must cancel first |
| Normal attacks during perf? | Blocked (except Musical Strike/Slinging Arrow) |
| Song overlap (two Bards) | Intersection becomes Dissonance Lv1 |
| ASPD stacking (Haste2) | ACoS, AR, THQ, SQ mutually exclusive — strongest only |
| ASPD stacking (cross-group) | Haste2 + ASPD Potions stack |
| ACoS affects bow users? | No |

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/index.js` | A1 (auto-cancel), A5 (caster exclusion), B2 (movement), B4 (weapon swap), B5 (Dissonance MISC), B6 (freeze duration), B7 (party freeze chance), B8 (bow exclusion), B9 (Encore clear), B10 (Pang Voice rate), C1 (silence fix), C2 (Dispel cancel), C3 (heavy damage cancel) |
| `server/src/ro_buff_system.js` | A4 (Haste2 exclusion group) |
| `server/src/ro_skill_data_2nd.js` | A2 (remove Unbarring Octave), A3 (Musical Strike damage), B1 (Whistle duration), B3 (ensemble prereqs x16), C5 (Siegfried values), C6 (Loki's Veil SP) |

---

## Verification Checklist (ALL IMPLEMENTED)

### Solo Skills
- [x] **Music Lessons**: +3 ATK/lv with instrument, song formula scaling
- [x] **Musical Strike**: 100/140/180/220/260% ATK, arrow consumption, usable during performance
- [x] **Adaptation**: Cancel performance, 5s first-cast window, SP deduction
- [x] **Encore**: Half SP cost, clears remembered skill after use, weapon check
- [x] **Dissonance**: Damage every 3s, MISC damage (no DEF/MDEF), zone filtering
- [x] **Frost Joker**: Screen freeze friend/foe, boss immune, ~12s duration, party half-chance, BLOCKED during performance
- [x] **A Whistle**: 180s duration, FLEE+PD buff, linger 20s, caster excluded
- [x] **Assassin Cross**: ASPD buff, Haste2 exclusion, no bow users, caster excluded
- [x] **Poem of Bragi**: Cast time + ACD reduction, caster excluded
- [x] **Apple of Idun**: MaxHP% + HP regen every 6s, caster excluded
- [x] **Pang Voice**: Formula-based success rate, boss immune, quest skill, BLOCKED during performance

### System Behaviors
- [x] Starting new performance auto-cancels old (no error)
- [x] Movement speed 25-50% during solo performance (based on Music Lessons)
- [x] Weapon swap cancels performance instantly
- [x] Stun/Freeze/Stone cancel performance; Silence does NOT (solo)
- [x] Dispel cancels performance
- [x] 25%+ MaxHP single hit cancels performance
- [x] SP depletion / duration expiry / death / disconnect cancel performance
- [x] Caster does NOT receive own song buff
- [x] All non-caster players in AoE receive buff
- [x] Buff linger 20s after leaving AoE
- [x] ACoS does not affect bow users
- [x] ACoS/AR/THQ/SQ only strongest applies (Haste2 exclusion)
- [x] Encore clears remembered skill after use (no chaining)
- [x] Frost Joker/Scream/Pang Voice BLOCKED during performance (rAthena AllowWhenPerforming)
- [x] Song+Song overlap → Dissonance Lv1 MISC damage (Bug #19)

### Data Corrections
- [x] Musical Strike: 100/140/180/220/260%
- [x] A Whistle duration: 180000ms
- [x] Unbarring Octave (1510): removed (non-canonical — iRO name for Frost Joker)
- [x] All 16 ensemble prerequisites corrected (8 Bard + 8 Dancer)
- [x] Loki's Veil SP: 15 (both Bard + Dancer)
- [x] Invulnerable Siegfried: element resist 60-80%, status resist 10-50%
