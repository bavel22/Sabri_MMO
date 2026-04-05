# Knight Skills Comprehensive Audit & Fix Plan (v2 — Deep Research Verified)

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Knight_Class_Research](Knight_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-16
**Scope:** All 11 Knight skills (IDs 700-710), comparing current implementation against RO Classic pre-renewal mechanics.
**Method:** Cross-referenced implementation code against 4 deep research agents querying iRO Wiki Classic, RateMyServer, Divine Pride, rAthena pre-renewal source code (skill_db.yml, skill_tree.yml, .cpp source), and jRO official data.

---

## Implementation Status -- ALL FIXES COMPLETE (2026-03-16, updated 2026-03-20)

All 21 issues from the deep-research audit have been implemented:
- 6 Critical: ALL FIXED
- 7 High: ALL FIXED
- 5 Medium: ALL FIXED (including previously deferred #12 and #14)
- 2 Low: ALL FIXED
- 1 Systemic: FIXED

### Additional Fixes (2026-03-20 — rAthena source verification pass)
- **Spear Stab AoE**: Rewritten to scan 4 cells from target toward caster (rAthena `spearstab.cpp`), was scanning entire caster-to-target line
- **Bowling Bash AoE**: Complete rewrite — 8-dir knockback, cell-by-cell splash, chain depth formula `c=floor((lv-depth+1)/2)`, self-collision double-hit, boss immunity, random chain direction
- **Brandish Spear targeting**: Fixed `targetType: 'aoe'` → `'single'` so client sends enemy targetId
- **Charge Attack crashes**: Fixed DB column names (`id`→`character_id`, `position_x`→`x`), added `player.lastX/lastY` memory update
- **THQ/SQ/AR stats window**: Added `player:stats` emit after buff application
- **Equip handler**: Moved `player:stats` emit after buff strip so weapon swap correctly removes ASPD
- **`/mount` chat command**: Added as trigger for mount toggle
- **Abracadabra Knight IDs**: Fixed shifted IDs 700-706 → correct 701-707

### Already Working (verified post-audit, were listed as deferred but actually exist)
- THQ non-stacking with Adrenaline Rush -- ASPD mutual exclusion at line 10829 strips conflicting buffs before applying THQ (same for SQ and AR)
- Dispel removes THQ -- Dispel handler (line 12634) UNDISPELLABLE set does NOT include two_hand_quicken, so it is correctly removed

### Remaining Deferred Items (blocked by unimplemented systems)
- Auto Counter facing requirement -- needs facing/direction system (most private servers skip this)
- One-Hand Quicken (711) -- needs Soul Linker class to cast Knight Spirit buff (OHQ was introduced WITH Soul Linker in Episode 10.3, never existed as standalone)
- Charge Attack trap escape -- needs Ankle Snare/Fiber Lock/Close Confine escape logic
- Riding indoor restriction -- needs zone flag system (`isIndoor`)
- Quest skill gating (Charge Attack) -- needs quest system (Knight Platinum Skills Quest)

### Cross-System Improvements Made
- `calculatePhysicalDamage()` wrapper: now passes isMounted, all card fields, target modeFlags/templateId/subRace/cardDefMods
- Weapon skill no-crit: affects ALL weapon skills project-wide (not just Knight)
- ignoreDefense option: available for any future skill that needs DEF bypass
- isRanged Pneuma check: available for any future ranged physical skill using executePhysicalSkillOnEnemy()

---

## Executive Summary

**11 skills total** -- 3 passives, 8 actives. All have handlers implemented. Deep research revealed significantly more issues than the initial audit.

| Category | Count |
|----------|-------|
| Total individual issues found | 21 |
| Critical (breaks RO Classic core behavior) | 6 |
| High (missing important interaction) | 7 |
| Medium (missing edge case / secondary mechanic) | 5 |
| Low (cosmetic / minor data) | 2 |
| Systemic (affects all weapon skills, not just Knight) | 1 |

---

## Per-Skill Audit Results

---

### Skill 700: Spear Mastery (Passive) — PASS

**Sources:** iRO Wiki Classic, rAthena pre-re skill_db
**Handler:** `getPassiveSkillBonuses()` lines 611-617

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Weapon types | 1H Spear + 2H Spear | `spear`, `one_hand_spear`, `two_hand_spear` | CORRECT |
| Dismounted bonus | +4 ATK per level | `smLv * 4` | CORRECT |
| Mounted bonus | +5 ATK per level | `player.isMounted ? 5 : 4` | CORRECT |
| Mastery ATK bypasses DEF | Yes (added after DEF reduction in formula) | PassiveATK is post-DEF | CORRECT |
| Shared with Crusader | Yes | `sharedClasses: ['crusader']` | CORRECT |

**Verdict: No issues.**

---

### Skill 701: Pierce (Active) — 3 ISSUES

**Sources:** iRO Wiki Classic, RateMyServer, Divine Pride, rAthena pre-re source
**Handler:** Lines 9509-9615

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Damage formula | (100+10*Lv)% per hit | effectValue 110-200 | CORRECT |
| Multi-hit by size | Small=1, Med=2, Large=3 | Implemented | CORRECT |
| **Damage bundling** | **Single calc × hit count (ONE packet)** | **Independent rolls per hit** | **ISSUE #1** |
| **Critical** | **Cannot crit (weapon skill)** | **Crit roll is active** | **ISSUE #2** |
| **Lex Aeterna** | **Doubles ENTIRE bundled damage** | **Consumes on first hit only** | **ISSUE #3** |
| HIT bonus | ×(1 + 5*Lv/100) multiplicative | `learnedLevel * 5` multiplicative | CORRECT |
| SP cost | 7 flat | 7 | CORRECT |
| Weapon req | Spear (1H or 2H) | `isSpearWeapon()` | CORRECT |
| Prerequisites | Spear Mastery Lv1 only | `[{700, 1}]` | CORRECT |

#### Issue #1 — Pierce uses independent rolls instead of bundled damage [CRITICAL]

**Problem:** In RO Classic, Pierce calculates damage ONCE, then multiplies by the size-based hit count. It's a single damage packet. The current implementation calls `calculatePhysicalDamage()` independently for each hit (2-3 times for Med/Large), which means each hit can miss independently, has different variance, etc. This is wrong.

**Verified by:** rAthena source — `Hit: Multi_Hit`, `HitCount: 3`, size-based div set at runtime via `tstatus->size + 1`. iRO Wiki Classic confirms all hits appear as one damage number.

**Fix:** Replace the per-hit loop with a single damage calculation multiplied by hit count:
```js
const hitCount = PIERCE_HITS_BY_SIZE[targetSize] || 1; // small=1, medium=2, large=3
const result = calculateSkillDamage(..., effectVal, ...);
if (!result.isMiss) {
    result.damage *= hitCount;
}
```

#### Issue #2 — Pierce can critical (weapon skills cannot crit in pre-renewal) [SYSTEMIC]

**Problem:** `calculatePhysicalDamage()` always rolls for crits (line 488-507 in ro_damage_formulas.js). In pre-renewal RO, offensive weapon skills CANNOT critical — only auto-attacks and a few specific exceptions (Sharp Shooting). This affects ALL weapon skills, not just Pierce.

**Fix (systemic):** In `calculatePhysicalDamage()`, skip crit roll when `isSkill === true && !forceCrit`:
```js
if (forceCrit) {
    isCritical = true;
} else if (!isSkill) {  // Only auto-attacks can crit naturally
    // existing crit roll logic
}
```

**Impact:** Fixes Pierce, Spear Stab, Brandish Spear, Bowling Bash, Spear Boomerang, Charge Attack, and ALL other weapon skills project-wide.

#### Issue #3 — Lex Aeterna consumed on first hit instead of doubling entire bundle [HIGH]

**Problem:** Since Pierce damage should be bundled (Issue #1), Lex Aeterna should double the ENTIRE damage (all hits). Currently LA is consumed after the first hit, so only 1/3 of the damage is doubled for Large targets.

**Fix:** Once Issue #1 is fixed (single damage calc × hit count), the existing LA logic in `executePhysicalSkillOnEnemy()` at line 1278 will naturally double the entire bundled amount. No separate fix needed — this resolves automatically when #1 is fixed.

---

### Skill 702: Spear Stab (Active) — 1 ISSUE (FIXED 2026-03-20)

**Sources:** iRO Wiki Classic, Divine Pride, rAthena pre-re source (`spearstab.cpp`)
**Handler:** Lines ~12962-13057

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Damage formula | (100+20*Lv)% ATK | effectValue 120-300 | CORRECT |
| Line AoE direction | 4 cells from **target toward caster** | Scans target→caster, 200 UE depth | FIXED (2026-03-20) |
| Line AoE width | 1 cell wide (`map_foreachincell`) | 75 UE perp tolerance (continuous coords) | FIXED (2026-03-20) |
| Zone filtering | Same zone only | `enemy.zone !== ssZone` | CORRECT |
| Knockback | 6 cells backward | `knockbackTarget()` with 6 | CORRECT |
| Lex Aeterna | Per-target | Consumed per target | CORRECT |
| Cannot crit | Weapon skill rule | Covered by Issue #2 (systemic) | SYSTEMIC |

**Fix applied (2026-03-20):** AoE was scanning entire caster-to-target line. Rewritten to match rAthena: scan 4 cells from target position back toward caster using perpendicular-distance-from-line detection (75 UE width for continuous coordinates).

---

### Skill 703: Brandish Spear (Active) — 2 ISSUES (ALL FIXED)

**Sources:** iRO Wiki Classic, rAthena brandishspear.cpp source code
**Handler:** Lines 9717-9813

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Mount requirement | Must be mounted | `if (!player.isMounted)` | CORRECT |
| Weapon req | Spear (1H or 2H) | `isSpearWeapon()` | CORRECT |
| Cast time | 700ms uninterruptible | castTime: 700 | CORRECT |
| After-cast delay | 1000ms | afterCastDelay: 1000 | CORRECT |
| AoE depth by level | Lv1-3=2 rows, 4-6=3, 7-9=4, 10=5 | Level brackets | CORRECT |
| AoE width | 5 cells (3 for D row at Lv10) | 250 UE total | CORRECT |
| Knockback | 2 cells | `knockbackTarget()` with 2 | CORRECT |
| **Zone-based damage multiplier** | **Inner zones get bonus damage** | **Flat damage for all** | **ISSUE #4** |
| Cannot crit | Weapon skill rule | Covered by #2 systemic | SYSTEMIC |
| DEF reduction during cast | NOT in pre-renewal (rAthena confirms) | Not implemented | CORRECT (not needed) |

#### Issue #4 — Brandish Spear missing zone-based damage multiplier [HIGH]

**Problem:** In RO Classic (confirmed by rAthena brandishspear.cpp source), targets closer to the caster receive significantly MORE damage via a zone-based multiplier system. The current implementation applies flat damage to all targets in the AoE.

**Zone damage table (from rAthena source):**

| Level | Zone 0 (closest) | Zone 1 (Lv4+) | Zone 2 (Lv7+) | Zone 3 (Lv10) |
|-------|-------------------|----------------|----------------|----------------|
| 1-3 | base (120-160%) | — | — | — |
| 4 | base + base/2 = 270% | 180% | — | — |
| 7 | base + base/2 + base/4 = 420% | base + base/2 = 360% | 240% | — |
| 10 | 300+150+75+37 = **562%** | 300+150+75 = **525%** | 300+150 = **450%** | **300%** |

The formula:
```js
let totalRatio = baseRatio; // 100 + 20*lv
if (zone === 0) {
    if (lv > 3) totalRatio += Math.floor(baseRatio / 2);
    if (lv > 6) totalRatio += Math.floor(baseRatio / 4);
    if (lv > 9) totalRatio += Math.floor(baseRatio / 8);
} else if (zone === 1) {
    if (lv > 6) totalRatio += Math.floor(baseRatio / 2);
    if (lv > 9) totalRatio += Math.floor(baseRatio / 4);
} else if (zone === 2) {
    if (lv > 9) totalRatio += Math.floor(baseRatio / 2);
}
// zone 3: just base ratio
```

**Fix:** In the Brandish Spear handler, when iterating enemies in the AoE, calculate which zone each enemy falls in based on their forward distance from the caster. Apply the zone-based damage multiplier to each enemy's damage. **IMPLEMENTED.**

#### Issue #4b — Brandish Spear targetType prevents enemy targeting [HIGH] (FIXED 2026-03-20)

**Problem:** `targetType: 'aoe'` in `ro_skill_data_2nd.js` caused the client to use ground-click targeting, sending no `targetId`. The handler requires an enemy target to calculate AoE direction.

**Fix:** Changed `targetType` from `'aoe'` to `'single'` in `ro_skill_data_2nd.js`. The handler calculates AoE from caster toward the clicked enemy.

---

### Skill 704: Spear Boomerang (Active) — 2 ISSUES

**Sources:** iRO Wiki Classic, rAthena pre-re skill_db, Pneuma wiki page
**Handler:** Lines 9820-9832

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Damage formula | (100+50*Lv)% ATK | effectValue 150-350 | CORRECT |
| Per-level range | 3/5/7/9/11 cells (rAthena pre-re) | `(1+2*lv)*50` UE = 150-550 | CORRECT |
| After-cast delay | 1000ms | afterCastDelay: 1000 | CORRECT |
| Weapon req | Spear (1H or 2H) | `isSpearWeapon()` | CORRECT |
| **Ranged physical / Pneuma** | **Blocked by Pneuma** | **No Pneuma check** | **ISSUE #5** |
| Data range field | Max 11 cells = 550 UE | `range: 600` | **ISSUE #6** |
| Knockback | None (no source mentions it) | None | CORRECT |

#### Issue #5 — Spear Boomerang not blocked by Pneuma [HIGH]

**Verified by:** iRO Wiki Pneuma page explicitly lists Spear Boomerang. rAthena classifies it as ranged physical.

**Fix:** Add `isRanged: true` option to `executePhysicalSkillOnEnemy()` and add Pneuma ground effect check. (See Cross-System Issue #13 below.)

#### Issue #6 — Data range field 600 should be 550 [LOW]

**Fix:** `ro_skill_data_2nd.js` line 18: change `range: 600` to `range: 550`.

---

### Skill 705: Two-Hand Quicken (Active) — 3 ISSUES

**Sources:** iRO Wiki Classic (THQ + ASPD pages), RateMyServer, rAthena pre-re
**Handler:** Lines 9835-9855

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| ASPD bonus | +30% (SM=0.3 in pre-renewal ASPD formula) | `aspdIncrease: 30` | CORRECT |
| Weapon req | 2H Sword | `thqWeapon !== 'two_hand_sword'` | CORRECT |
| Duration | Lv×30s (30-300s) | `learnedLevel * 30000` | CORRECT |
| SP cost | 10+Lv×4 (14-50) | `14+i*4` | CORRECT |
| Prereq | 2H Sword Mastery Lv1 (101) | `[{101, 1}]` | CORRECT |
| Quagmire cancels | Yes | Line 19890 strips it | CORRECT |
| **CRI bonus** | **+2+Lv (Lv1=+3, Lv10=+12)** | **NOT implemented** | **ISSUE #7** |
| **HIT bonus** | **+2×Lv (Lv1=+2, Lv10=+20)** | **NOT implemented** | **ISSUE #7** |
| **Cancel on weapon change** | **Must cancel if 2H unequipped** | **NOT implemented** | **ISSUE #8** |
| **Decrease AGI cancels** | **Must cancel all quicken buffs** | **NOT implemented** | **ISSUE #9** |
| Dispel removes | Yes | Need to verify Dispel handler | VERIFY |
| Stacking | Does NOT stack with Adrenaline Rush/Spear Quicken (same category, highest wins) | Stacking rules unknown | MEDIUM |

#### Issue #7 — Two-Hand Quicken missing CRI and HIT bonuses [HIGH]

**Verified by:** iRO Wiki Classic THQ page, RateMyServer, Divine Pride — ALL agree these bonuses exist in pre-renewal.
- CRI: `2 + level` (Lv1=+3, Lv10=+12)
- HIT: `2 * level` (Lv1=+2, Lv10=+20)

**Fix:** Add `critBonus` and `hitBonus` fields to the buff application:
```js
applyBuff(player, {
    skillId, name: 'two_hand_quicken', ...,
    aspdIncrease: 30,
    critBonus: 2 + learnedLevel,    // +3 to +12
    hitBonus: 2 * learnedLevel,      // +2 to +20
    duration: thqDuration
});
```
Then in `getBuffModifiers()` / `ro_buff_system.js`, apply these bonuses to derived stats.

#### Issue #8 — Two-Hand Quicken NOT cancelled on weapon change [HIGH]

**Verified by:** Divine Pride: "knocked off by changing weapons except two-handed". iRO Wiki Classic confirms.

**Fix:** In `inventory:equip` handler (after line ~16085), check and strip quicken buffs when weapon type changes:
```js
if (hasBuff(player, 'two_hand_quicken') && player.weaponType !== 'two_hand_sword') {
    removeBuff(player, 'two_hand_quicken');
    broadcastToZone(equipZone, 'skill:buff_removed', { targetId: characterId, isEnemy: false, buffName: 'two_hand_quicken', reason: 'weapon_change' });
}
// Same pattern for one_hand_quicken and spear_quicken
```

#### Issue #9 — Decrease AGI does NOT cancel quicken buffs [HIGH]

**Verified by:** iRO Wiki Classic, RateMyServer, Divine Pride — ALL confirm Decrease AGI removes Quicken buffs.

**Fix:** In Decrease AGI handler (after line 8348), add:
```js
for (const qBuff of ['two_hand_quicken', 'one_hand_quicken', 'spear_quicken']) {
    if (hasBuff(target, qBuff)) {
        removeBuff(target, qBuff);
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: qBuff, reason: 'decrease_agi' });
    }
}
```

---

### Skill 706: Auto Counter (Active) — 3 ISSUES

**Sources:** iRO Wiki Classic, RateMyServer, rAthena pre-re skill_db + skill.conf
**Handler (skill use):** Lines 9860-9880
**Handler (combat hook):** Lines 20752-20794

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| SP cost | 3 flat | 3 | CORRECT |
| Duration | Lv×400ms (400-2000ms) | `learnedLevel * 400` | CORRECT |
| Melee-only trigger | Only auto-attacks, not skills | Range check in hook | CORRECT |
| Block incoming damage | Completely blocks | `damage = 0` | CORRECT |
| Guaranteed critical counter | Yes (100% crit for default config) | `forceCrit: true` | CORRECT |
| One counter per activation | Yes | Buff removed after trigger | CORRECT |
| Can counter boss monsters | Yes | No boss immunity check | CORRECT |
| Weapon restriction | Any weapon except Bow | No restriction | OK (close enough) |
| **Prerequisite** | **2H Sword Mastery Lv1 (101)** | **THQ Lv1 (705)** | **ISSUE #10** |
| **Counter ignores DEF** | **Yes (rAthena: IgnoreDefense flag)** | **Normal DEF calc** | **ISSUE #11** |
| Active counter (click) | 2x crit rate + 20 HIT | NOT implemented | **ISSUE #12** |
| Facing requirement | Must face attacker | Not checked | MEDIUM (defer) |

#### Issue #10 — Auto Counter has WRONG prerequisite [CRITICAL]

**Verified by:** iRO Wiki Classic, RateMyServer, rAthena pre-re/skill_tree.yml — ALL say "Two-Handed Sword Mastery Lv1", NOT "Two-Hand Quicken Lv1".

**Fix:** In `ro_skill_data_2nd.js` line 20, change:
```
prerequisites: [{ skillId: 705, level: 1 }]
→
prerequisites: [{ skillId: 101, level: 1 }]
```

#### Issue #11 — Auto Counter does NOT ignore DEF [CRITICAL]

**Verified by:** rAthena pre-re skill_db.yml: `DamageFlags: { IgnoreDefense: true, Critical: true }`. iRO Wiki Classic Stats page: "pre-renewal criticals fully ignore enemy DEF, both % and pure value part". RateMyServer: "Skill ignores target's defense".

**Current:** The counter-attack uses `forceCrit: true` which makes it a critical, but `calculatePhysicalDamage()` still applies DEF reduction even for crits. In pre-renewal, criticals bypass ALL DEF (hard + soft).

**Fix:** Add `ignoreDefense: true` to the Auto Counter counter-attack options:
```js
const counterResult = calculatePhysicalDamage(counterAtkInfo, counterDefInfo, {
    isSkill: false, forceCrit: true, forceHit: true, ignoreDefense: true
});
```
Also ensure `calculatePhysicalDamage()` respects `ignoreDefense` option (skip hard DEF and soft DEF when true).

**Note:** This also means all pre-renewal criticals should ignore DEF. This is a systemic issue but Auto Counter is the most impacted since it's a guaranteed crit.

#### Issue #12 — Active counter (click-to-attack during stance) NOT implemented [MEDIUM]

**Verified by:** iRO Wiki Classic: clicking target during stance gives 2× crit rate + 20 HIT (NOT guaranteed crit).

**Fix (defer):** Lower priority than passive counter which is the primary mechanic. Can add later by checking `hasBuff(player, 'auto_counter')` in attack handler.

---

### Skill 707: Bowling Bash (Active) — FULL REWRITE (2026-03-20)

**Sources:** iRO Wiki Classic, rAthena `bowlingbash.cpp` source (lines 43-115), Hercules `skill.c` (lines 5308-5397), pre-re `skill_db.yml`
**Handler:** Lines ~13272-13405

| Aspect | RO Classic (rAthena source) | Current | Status |
|--------|----------------------|---------|--------|
| Damage formula | (100+40*Lv)% ATK | effectValue 140-500 | CORRECT |
| SP cost | 12+Lv (13-22) | `13+i` | CORRECT |
| Cast time | 700ms uninterruptible | castTime: 700 | CORRECT |
| Two-hit mechanic | HitCount: 1 — extra hit only on collision | 1 hit + self-collision bonus | FIXED (2026-03-20) |
| Lex Aeterna | Consumed on first hit of target | Consumed on first `bbDealDamage` call | CORRECT |
| Knockback distance | `c = floor((lv - depth + 1) / 2)` | Same formula | FIXED (2026-03-20) |
| Knockback direction | Primary: away from caster. Chain: random 8-dir | 8-dir system, `rnd()%8` for chain | FIXED (2026-03-20) |
| 3x3 splash | Cell-by-cell KB with splash check at each step | Same — stops on collision | FIXED (2026-03-20) |
| Chain reaction | Recursive — splashed enemies get own chain | `bbChainReaction()` recursive | FIXED (2026-03-20) |
| Boss immunity | Bosses take damage but no knockback/chain | `modeFlags.isBoss` check | FIXED (2026-03-20) |
| Hit tracking | `bowling_db` prevents double-hits | `bbHitSet` Set | CORRECT |
| No weapon restriction | Any weapon | No weapon check | CORRECT |
| Cannot crit | Weapon skill rule | Covered by #2 systemic | SYSTEMIC |
| Gutter line | Official but optional (many servers disable) | Not implemented | INTENTIONAL |

**Full rewrite (2026-03-20):** Previous implementation used unconditional 2-hit, fixed 1-cell knockback, and `knockbackTarget()` for chain. Rewritten to match rAthena source: 8-direction knockback system, cell-by-cell splash checking, self-collision double-hit, chain depth formula, boss immunity, random direction for chain targets.

**Priority:** MEDIUM — defer unless explicitly requested. The basic 2-hit + splash works correctly.

---

### Skill 708: Riding (Passive) — 1 MINOR ISSUE

**Sources:** iRO Wiki Classic, Movement Speed wiki
**Handler:** `mount:toggle` socket event lines 5406-5455

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| Toggle | Mount/dismount toggle | `player.isMounted = !player.isMounted` | CORRECT |
| Class restriction | Knight/LK/Crusader/Paladin | Checked | CORRECT |
| Weight bonus | +1000 | +1000 | CORRECT |
| ASPD penalty | 50% base | Applied in `calculateASPD()` | CORRECT |
| Prerequisites | Endure Lv1 (106) | `[{106, 1}]` | CORRECT |
| **Speed bonus** | **~36% (0.15s→0.11s per cell)** | **25%** | **ISSUE #15** |
| Mounted spear vs Medium | 100% (overrides 75% penalty) | Not in damage formula | Covered by #16 |

#### Issue #15 — Mount speed bonus is 25% but RO Classic is ~36% [LOW]

**Verified by:** iRO Wiki Movement Speed: base walking 0.15s/cell, mounted 0.11s/cell. Speed increase = 0.15/0.11 ≈ 1.364 = +36.4%.

**Fix:** Change mount speed multiplier from 1.25 to 1.36 at line 2998. Low priority — 25% is a common private server simplification.

---

### Skill 709: Cavalier Mastery (Passive) — PASS

**Sources:** iRO Wiki Classic, rAthena source
**Handler:** `getPassiveSkillBonuses()` lines 619-621 + `calculateASPD()`

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| ASPD recovery | 10%/lv (Lv5 = full restore) | `0.5 + cavalierMasteryLv * 0.1` | CORRECT |
| Prerequisites | Riding Lv1 (708) | `[{708, 1}]` | CORRECT |
| Shared with Crusader | Yes | `sharedClasses: ['crusader']` | CORRECT |

**Verdict: No issues.**

---

### Skill 710: Charge Attack (Active/Quest) — 4 ISSUES + 2 CRASH FIXES (2026-03-20)

**Sources:** iRO Wiki Classic, rAthena issue #417 (jRO official data), rAthena pre-re source
**Handler:** Lines 10031-10058

| Aspect | RO Classic (verified) | Current | Status |
|--------|----------------------|---------|--------|
| SP cost | 40 flat | 40 | CORRECT |
| Quest skill | Yes | `questSkill: true` | CORRECT |
| Range | 14 cells = 700 UE | range: 700 | CORRECT |
| No weapon restriction | Any weapon | No check | CORRECT |
| **Distance damage tiers** | **0-3/4-6/7-9/10-12/13-14 cells** | **0-2/3-5/6-8/9-11/12+ cells** | **ISSUE #17** |
| **Distance cast time** | **500/1000/1500/1500/1500ms** | **Flat 1000ms** | **ISSUE #18** |
| **Caster rushes to target** | **Relocate even on miss/Pneuma** | **NOT implemented** | **ISSUE #19** |
| **Knockback** | **1 cell random direction** | **NOT implemented** | **ISSUE #20** |
| Ranged physical / Pneuma | Blocked by Pneuma (caster still moves) | No Pneuma check | Covered by #13 |

#### Issue #17 — Charge Attack distance damage tiers are WRONG [CRITICAL]

**Verified by:** rAthena issue #417 with jRO official data:

| Distance | Correct (jRO) | Current (wrong) |
|----------|---------------|-----------------|
| 0-3 cells | 100% | 0-2: 100% |
| 4-6 cells | 200% | 3-5: 200% |
| 7-9 cells | 300% | 6-8: 300% |
| 10-12 cells | 400% | 9-11: 400% |
| 13-14 cells | 500% | 12+: 500% |

**Fix:** Update the distance brackets in the handler:
```js
const distCells = Math.floor(caDist / 50);
if (distCells <= 3) caEffectVal = 100;
else if (distCells <= 6) caEffectVal = 200;
else if (distCells <= 9) caEffectVal = 300;
else if (distCells <= 12) caEffectVal = 400;
else caEffectVal = 500;
```

#### Issue #18 — Charge Attack cast time is flat instead of distance-based [CRITICAL]

**Verified by:** rAthena source: `k = cap_value((distance-1)/3, 0, 2)`, casttime = base×(1+k). Base 500ms → 500/1000/1500ms.

**Fix:** Add special case before cast time processing in skill:use:
```js
if (skill.name === 'charge_attack' && targetId && isEnemy) {
    const caTarget = enemies.get(targetId);
    if (caTarget && !caTarget.isDead) {
        const caPos = await getPlayerPosition(characterId);
        if (caPos) {
            const caDist = Math.sqrt((caPos.x-caTarget.x)**2 + (caPos.y-caTarget.y)**2);
            const caDistCells = Math.floor(caDist / 50);
            const k = Math.min(2, Math.max(0, Math.floor((caDistCells - 1) / 3)));
            levelData = { ...levelData, castTime: 500 * (1 + k) };
        }
    }
}
```

#### Issue #19 — Charge Attack does NOT move caster to target [CRITICAL]

**Verified by:** iRO Wiki Classic, rAthena source. Caster relocates to 1 cell adjacent to target even on miss or Pneuma block. Exception: no relocation during WoE.

**Fix:** After `executePhysicalSkillOnEnemy()` returns, update caster position:
```js
const caEnemy = enemies.get(targetId);
if (caEnemy) {
    // Move to 1 cell adjacent to target
    const dx = caEnemy.x - attackerPos.x, dy = caEnemy.y - attackerPos.y;
    const dist = Math.sqrt(dx*dx + dy*dy);
    const nx = dist > 0 ? dx/dist : 0, ny = dist > 0 ? dy/dist : 0;
    const newX = caEnemy.x - nx * 50, newY = caEnemy.y - ny * 50; // 1 cell away
    player.lastX = newX; player.lastY = newY; player.lastZ = caEnemy.z || player.lastZ || 0;
    await pool.query('UPDATE characters SET x=$1, y=$2 WHERE character_id=$3', [newX, newY, characterId]);
    broadcastToZone(zone, 'player:moved', { characterId, x: newX, y: newY, z: player.lastZ, isTeleport: true });
}
```
**Note (2026-03-20):** Original code had `position_x`/`position_y` (don't exist) and `WHERE id` (should be `character_id`). Both crash bugs fixed. Also added `player.lastX/lastY` in-memory update.

#### Issue #20 — Charge Attack missing knockback [MEDIUM]

**Verified by:** rAthena source: `Knockback: 1`. Pre-renewal knockback is 1 cell in random direction.

**Fix:** After damage, knockback in random direction:
```js
if (!result.isMiss && caEnemy && !caEnemy.isDead) {
    const randAngle = Math.random() * 2 * Math.PI;
    const kbFromX = caEnemy.x - Math.cos(randAngle) * 50;
    const kbFromY = caEnemy.y - Math.sin(randAngle) * 50;
    knockbackTarget(caEnemy, targetId, kbFromX, kbFromY, 1, zone);
}
```

---

## Cross-System Issues

### Issue #13 — `executePhysicalSkillOnEnemy()` has NO Pneuma check [HIGH]

**Verified by:** iRO Wiki Pneuma page lists Spear Boomerang. rAthena classifies both Spear Boomerang and Charge Attack as ranged physical.

**Affected skills:** Spear Boomerang (704), Charge Attack (710), and any future ranged physical skill using this helper.

**Fix:** Add `isRanged` option to `executePhysicalSkillOnEnemy()`:
```js
if (options.isRanged) {
    const pneumas = getGroundEffectsAtPosition(targetPos.x, targetPos.y, targetPos.z || 0, 100);
    if (pneumas.find(e => e.type === 'pneuma')) {
        // SP consumed, skill goes on cooldown, but damage = 0
        player.mana = Math.max(0, player.mana - spCost);
        applySkillDelays(characterId, player, skillId, levelData, socket);
        broadcastToZone(zone, 'skill:effect_damage', { ..., damage: 0, isMiss: true, hitType: 'pneuma' });
        socket.emit('skill:used', { ... });
        return null;
    }
}
```

Update callers: `{ range: sbmRange, isRanged: true }` for Spear Boomerang, `{ range: caRange, isRanged: true }` for Charge Attack.

**Special for Charge Attack:** Caster STILL relocates even when Pneuma blocks damage.

### Issue #16 — Mounted spear vs Medium size penalty override missing [MEDIUM]

**Verified by:** iRO Wiki Classic Riding page: "Spear weapons deal 100% damage to Medium size while mounted" (overrides normal 75% penalty).

**Fix:** In `calculatePhysicalDamage()` in ro_damage_formulas.js, after size penalty lookup, add:
```js
if (attackerInfo.isMounted && isSpearWeapon(attackerInfo.weaponType) && targetSize === 'medium') {
    sizePenaltyPct = 100;
}
```

### Issue #2 (Systemic) — Weapon skills can critical in pre-renewal [SYSTEMIC]

**Verified by:** iRO Wiki Classic Stats page: "Offensive Skills do not take CRIT into account except for a few exceptions" (Sharp Shooting, etc.).

**Fix:** In `calculatePhysicalDamage()` (ro_damage_formulas.js line 488-507):
```js
if (forceCrit) {
    isCritical = true;
} else if (!isSkill) {  // Only auto-attacks can crit naturally
    // existing crit roll...
}
```

**Impact:** All weapon skills across all classes. Exceptions to re-enable: Sharp Shooting (Hunter), and any future skills with explicit crit mechanics.

---

## Fix Priority & Execution Order

### Phase 1: Critical Data Fixes (one-line changes)

| # | Issue | File | Change |
|---|-------|------|--------|
| 10 | Auto Counter prereq 705→101 | `ro_skill_data_2nd.js:20` | `skillId: 705` → `skillId: 101` |
| 6 | Spear Boomerang range 600→550 | `ro_skill_data_2nd.js:18` | `range: 600` → `range: 550` |

### Phase 2: Systemic Damage Pipeline Fix

| # | Issue | File | Change |
|---|-------|------|--------|
| 2 | Weapon skills cannot crit | `ro_damage_formulas.js:491` | Skip crit roll when `isSkill && !forceCrit` |

### Phase 3: Critical Skill Fixes

| # | Issue | File | Change |
|---|-------|------|--------|
| 1 | Pierce bundled damage | `index.js:9509-9615` | Replace per-hit loop with single calc × hit count |
| 17 | Charge Attack distance tiers wrong | `index.js:10046-10054` | Fix cell brackets (0-3/4-6/7-9/10-12/13-14) |
| 18 | Charge Attack flat cast time | `index.js` (pre-cast section) | Add distance-based cast time override |
| 19 | Charge Attack caster teleport | `index.js:10056` | Add position update + broadcast |
| 11 | Auto Counter ignores DEF | `index.js:20770+` + `ro_damage_formulas.js` | Add `ignoreDefense` option |

### Phase 4: High Priority Interaction Fixes

| # | Issue | File | Change |
|---|-------|------|--------|
| 7 | THQ missing CRI/HIT bonuses | `index.js:9845` + `ro_buff_system.js` | Add critBonus/hitBonus to buff |
| 8 | THQ not cancelled on weapon change | `index.js:~16085` | Add quicken cancel in equip handler |
| 9 | Decrease AGI doesn't cancel quicken | `index.js:8348` | Add quicken strip in Dec AGI handler |
| 5/13 | Pneuma check for ranged skills | `index.js:1228` | Add isRanged option to executePhysicalSkillOnEnemy |
| 4 | Brandish Spear zone damage | `index.js:9717-9813` | Add zone-based multiplier calculation |

### Phase 5: Medium Priority

| # | Issue | File | Change |
|---|-------|------|--------|
| 16 | Mounted spear vs Medium override | `ro_damage_formulas.js` | Size penalty check for mounted spear |
| 20 | Charge Attack knockback | `index.js:10056` | 1 cell random direction |
| 12 | Auto Counter active counter | `index.js` | 2x crit + 20 HIT on click |
| 14 | Bowling Bash chain reaction | `index.js:9885+` | Recursive splash on secondary knockback |

### Phase 6: Low Priority

| # | Issue | File | Change |
|---|-------|------|--------|
| 15 | Mount speed 25%→36% | `index.js:2998` | 1.25 → 1.36 |

---

## Files to Modify

| File | Changes |
|------|---------|
| `server/src/ro_skill_data_2nd.js` | Auto Counter prereq (line 20), Spear Boomerang range (line 18) |
| `server/src/ro_damage_formulas.js` | Weapon skill no-crit (systemic), mounted spear vs Medium, ignoreDefense option |
| `server/src/ro_buff_system.js` | THQ CRI/HIT bonus modifiers |
| `server/src/index.js` | Pierce bundled damage, Brandish zone multiplier, THQ weapon cancel in equip, Dec AGI quicken strip, Pneuma check in executePhysicalSkillOnEnemy, Charge Attack (tiers + cast time + teleport + knockback), Auto Counter ignoreDefense |

---

## Verification Checklist

- [x] **700 Spear Mastery:** +4 dismounted / +5 mounted, bypasses DEF
- [x] **701 Pierce:** Bundled damage (single calc x hit count), no crit, LA doubles all
- [x] **702 Spear Stab:** Line AoE 1 cell wide, knockback 6, no crit
- [x] **703 Brandish Spear:** Zone-based damage multiplier (inner=more), mount check, no crit
- [x] **704 Spear Boomerang:** Pneuma blocks it (isRanged), per-level range, no crit, data range 550
- [x] **705 Two-Hand Quicken:** +30% ASPD + CRI (+2+Lv) + HIT (+2*Lv), cancelled by weapon change/Dec AGI/Quagmire
- [x] **706 Auto Counter:** Correct prereq (101), counter ignores DEF (ignoreDefense), guaranteed crit, active counter mode (2x crit+20 HIT)
- [x] **707 Bowling Bash:** 2 hits, no crit, knockback 1 (fixed), chain reaction splash (recursive, max depth=skillLv)
- [x] **708 Riding:** +36% speed, +1000 weight, 50% ASPD penalty
- [x] **709 Cavalier Mastery:** 10%/lv ASPD recovery, Lv5 full
- [x] **710 Charge Attack:** Distance tiers (0-3/4-6/7-9/10-12/13-14), cast time (500/1000/1500), caster teleport, 1-cell knockback random, Pneuma blocks damage but not move
- [x] **Systemic:** Weapon skills cannot crit (isSkill skips crit roll), mounted spear vs Medium = 100%, ignoreDefense option, calculatePhysicalDamage wrapper passes card fields + isMounted

---

## Research Sources

All findings verified against:
- iRO Wiki Classic (irowiki.org/classic/)
- iRO Wiki Renewal (irowiki.org/wiki/) — for cross-reference only
- RateMyServer (ratemyserver.net)
- Divine Pride (divine-pride.net)
- rAthena Pre-Renewal source: `db/pre-re/skill_db.yml`, `db/pre-re/skill_tree.yml`
- rAthena C++ source: `bowlingbash.cpp`, `brandishspear.cpp`, `skill.conf`
- rAthena GitHub issues: #417 (Charge Attack jRO data), #3735 (Cavalier Mastery)
- jRO official documentation (via rAthena issue confirmations)
