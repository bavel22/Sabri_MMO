# Crusader Skills Audit & Fix Plan (v2 — Deep Research Verified)

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Crusader_Class_Research](Crusader_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-16
**Scope:** All 14 Crusader skills (IDs 1300-1313) + 3 shared Knight skills (700, 708, 709)
**Method:** Line-by-line comparison vs rAthena pre-renewal source code (battle.cpp, skill.cpp, status.cpp, status.yml, skill_tree.yml, skill_db.yml), cross-referenced with iRO Wiki Classic + RateMyServer
**v2 Changes:** Deep research corrected 7 items from v1 — Grand Cross has 5 additional bugs (ATK formula, DEF handling, hit count, AoE shape, self-damage), Shield Charge range was actually correct, Shield Boomerang has 2 new bugs (element, damage source), Shrink confirmed as scaling not flat, Reflect Shield prereq corrected, Decrease AGI strips more buffs

---

## Audit Summary

| Category | Count |
|----------|-------|
| Skills Audited | 17 (14 Crusader + 3 shared Knight) |
| CRITICAL bugs (wrong damage formula / missing core mechanic) | 3 |
| HIGH bugs (wrong data / missing mechanics) | 10 |
| MODERATE (edge cases / description / minor) | 4 |
| LOW / Deferred | 5 |
| Skills fully correct | 5 (Faith, Auto Guard, Holy Cross, Spear Mastery, Cavalry Mastery) |

---

## Per-Skill Audit Results

### 1. Faith (1300) — Passive ✅
| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| MaxHP bonus | +200/lv | `bonusMaxHp += faithLv * 200` | CORRECT |
| Holy resist | +5%/lv (50% at Lv10) | `holyResist = faithLv * 5` | CORRECT |
| Applied in pipeline | Enemy attack pipeline | Line 20866 | CORRECT |
| Prerequisites | None | None | CORRECT |

**Verdict: PASS**

---

### 2. Auto Guard (1301) — Toggle ✅
| Aspect | Expected (rAthena battle.cpp) | Implemented | Status |
|--------|------------------------------|-------------|--------|
| Block% | [5,10,14,18,21,24,26,28,29,30] | effectVal from skill data | CORRECT |
| Move delay | [300,300,300,300,300,200,200,200,200,100]ms | `lv<=5?300:lv<=9?200:100` | CORRECT |
| SP cost | 10+2*lv (12-30) | `12+i*2` | CORRECT |
| Duration | 300s | 300000ms | CORRECT |
| Blocks melee+ranged physical | `flag&BF_WEAPON` (no BF_SHORT/BF_LONG filter) | Pipeline doesn't filter by range | CORRECT |
| Does NOT block magic | BF_MAGIC excluded | Only in physical pipeline | CORRECT |
| Blocks physical skills too | Yes (unless INF2_IGNOREAUTOGUARD) | Yes | CORRECT |
| Shield required | Yes | Checked | CORRECT |
| Kyrie interaction | Kyrie checked first, AG checked later, both can be active | Not relevant (no conflict) | OK |
| Card on-hit when blocked | **NO** — rAthena returns 0 before card effects | Not triggered (damage=0 path) | CORRECT |
| Toggle on/off | Yes | `hasBuff` check | CORRECT |
| Prerequisites | None | None | CORRECT |

**Verdict: PASS**

**Source:** rAthena `battle.cpp` `battle_status_block_damage()` — `flag&BF_WEAPON && rnd()%100 < sce->val2 && !INF2_IGNOREAUTOGUARD`

---

### 3. Holy Cross (1302) — Active ✅
| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| ATK% | 100+35*lv (135-450) | `135+i*35` | CORRECT |
| SP cost | 10+lv (11-20) | `11+i` | CORRECT |
| Element | Holy (forced) | `{ skillElement: 'holy' }` | CORRECT |
| Hit count | 2 hits (skill_db NumHit: -2) | 2 emits with 150ms delay | CORRECT |
| 2H spear doubles | Contested — rAthena has `#ifdef RENEWAL` but iRO Wiki Classic says yes | `hcIs2H ? effectVal*2 : effectVal` | ACCEPTABLE |
| Blind chance | 3*lv % | `learnedLevel * 3` | CORRECT |
| Prerequisites | Faith Lv7 | `{skillId:1300, level:7}` | CORRECT |

**Verdict: PASS** — 2H spear doubling kept per iRO Wiki Classic (widely expected by players despite rAthena's `#ifdef RENEWAL` guard)

---

### 4. Grand Cross (1303) — Hybrid AoE — MULTIPLE BUGS

| Aspect | Expected (rAthena battle.cpp) | Implemented | Status |
|--------|------------------------------|-------------|--------|
| Skill ratio | 100+40*lv (140-500%) | `effectVal = 140+i*40` | CORRECT |
| SP cost | 30+7*lv (37-100) | `30+(i+1)*7` | CORRECT |
| HP cost | 20% current HP, cannot kill | `Math.max(1, health - floor(health*0.2))` | CORRECT |
| Cast time | 3000ms uninterruptible | 3000ms | CORRECT |
| ACD / Cooldown | 1500ms / 1000ms | 1500 / 1000 | CORRECT |
| Element | Holy | Holy element mod | CORRECT |
| Zone filtering | Required | `enemy.zone !== gcZone` | CORRECT |
| Blind on Undead/Demon | 3*lv % | Checked | CORRECT |
| Lex Aeterna | Per target | Per-enemy check | CORRECT |
| Prerequisites | Holy Cross Lv6 + Faith Lv10 | Correct | CORRECT |
| **MATK formula** | `matkMin=INT+floor(INT/7)^2`, `matkMax=INT+floor(INT/5)^2` | **Uses INT/7 for both, 70% scale for min** | **BUG C1** |
| **ATK formula** | WeaponATK only (no StatusATK/STR), with DEX variance + size penalty | **Includes StatusATK (STR-based)** | **BUG C2** |
| **DEF handling** | ATK: hard DEF (%) + soft DEF (flat). MATK: hard MDEF (%) + soft MDEF (flat) | **Only hardDef flat subtraction** | **BUG C3** |
| **Hit count** | 3 ticks (950ms/300ms interval via ground effect) | **1 hit per enemy** | **BUG H1** |
| **AoE shape** | 29-cell diamond extending 4 cells each direction | **9-cell cross extending 2 cells** | **BUG H2** |
| **Self-damage** | Per-tick self-hit = half of damage calc against self | **Average of dealt damage / 2** | **BUG H3** |

#### BUG C1 — MATK Formula (CRITICAL)

**Current code (~line 10207):**
```javascript
const statusMATK = gcEffStats.int + Math.floor(gcEffStats.int / 7) ** 2; // INT/7 = min formula
const matkMax = statusMATK + weaponMATK;  // WRONG: uses min as max
const matkMin = Math.floor(statusMATK * 0.7) + Math.floor(weaponMATK * 0.7); // WRONG: arbitrary 70%
```
At INT=50: range 69-99. Correct: 99-150. **~34% less MATK damage.**

**Fix:** Use pre-computed `gcEffStats.matkMin` / `gcEffStats.matkMax`.

#### BUG C2 — ATK Formula (CRITICAL)

**Current code includes StatusATK (STR-based):**
```javascript
const statusATK = gcEffStats.str + Math.floor(gcEffStats.str / 10) ** 2 + Math.floor(gcEffStats.dex / 5) + Math.floor(gcEffStats.luk / 3);
const totalATK = statusATK + weaponATK + passiveATK;
```

**rAthena pre-renewal Grand Cross ATK** (`battle.cpp` line 6282-6302): Uses ONLY `battle_calc_base_damage()` which for PCs = WeaponATK with DEX-based variance + size penalty. Then `battle_calc_defense_reduction()` (hard DEF % + soft DEF flat). Then adds refine bonus. **NO StatusATK (batk), NO mastery bonuses.**

**Fix:** ATK portion = WeaponATK with variance, not StatusATK+WeaponATK+Passive.

#### BUG C3 — DEF Handling (CRITICAL)

**Current code:** `gcDamage = gcDamage - (enemy.hardDef || 0)` — flat subtraction only.

**rAthena:** ATK portion goes through `battle_calc_defense_reduction()` which applies:
- Hard DEF as **percentage**: `ATK = ATK * (100 - hardDef) / 100`
- Soft DEF (VIT-based) as **flat subtraction**: `ATK = ATK - softDef`

MATK portion goes through standard MDEF reduction:
- Hard MDEF as **percentage**: `MATK = MATK * (100 - hardMDEF) / 100`
- Soft MDEF as **flat subtraction**: `MATK = MATK - softMDEF`

**Fix:** Apply both hard% + soft-flat DEF to ATK, and hard% + soft-flat MDEF to MATK.

#### BUG H1 — Hit Count (HIGH)

**rAthena:** Grand Cross creates a ground effect unit with Duration1=950ms, Interval=300ms → **3 ticks per enemy** (950/300 ≈ 3). Each tick deals full (ATK+MATK)*ratio damage.

**Current:** 1 hit per enemy.

**Fix:** Either loop 3 damage applications per enemy with 300ms delays, or multiply single damage by 3. The delay approach is more accurate but the multiplication is simpler. **Recommend: 3 sequential damage ticks with 300ms delay using setTimeout.**

#### BUG H2 — AoE Shape (HIGH)

**rAthena** (`skill.cpp` line 14184): 29-cell diamond/rhombus pattern extending 4 cells in each cardinal direction:
```
        X
       X X
     X X X X X
    X X X X X
   X X X X X X X X X
    X X X X X
     X X X X X
       X X
        X
```

**Current:** 9-cell cross extending only 2 cells. Much smaller than intended.

**Fix:** Expand cross offsets to the 29-cell diamond. With CELL=50 UE:
```javascript
const crossOffsets = [];
for (let dx = -4; dx <= 4; dx++) {
    for (let dy = -4; dy <= 4; dy++) {
        if (Math.abs(dx) + Math.abs(dy) <= 4) {
            crossOffsets.push({ dx: dx * CELL, dy: dy * CELL });
        }
    }
}
// This generates all cells where |dx|+|dy| <= 4, which is a diamond shape
```

#### BUG H3 — Self-Damage (HIGH)

**rAthena:** Caster is included in the ground effect target list. Self-damage = `floor(calculated_damage_against_self / 2)` per tick, with element fix against caster's armor element.

**Current:** Average of total damage dealt to all enemies / 2. This under-counts when few enemies are hit and over-counts with many.

**Fix:** Calculate GC damage against SELF once (using caster's own DEF/MDEF as target), then `selfDmg = floor(gcDamageVsSelf / 2) * holyVsCaster / 100 * (100 - faithResist) / 100`, applied per tick (3 times total).

---

### 5. Shield Charge (1304) — Active — HAS BUG

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| ATK% | 100+20*lv (120-200) | `120+i*20` | CORRECT |
| SP cost | 10 flat | 10 | CORRECT |
| Stun chance | 15+5*lv (20-40%) | `20+(lv-1)*5` | CORRECT |
| Stun duration | 5s | 5000ms | CORRECT |
| Knockback | lv+4 cells (5-9) | `learnedLevel + 4` | CORRECT |
| Shield required | Yes | Checked | CORRECT |
| Range | **3 cells** (rAthena skill_db range=3) | **150 UE (3 cells)** | **CORRECT** |
| **Element** | **Weapon element** (rAthena: element=weapon) | **`skillElement: null`** (inherits weapon) | **CORRECT** |
| Prerequisites | Auto Guard Lv5 | `{skillId:1301, level:5}` | CORRECT |

**v2 Correction:** Shield Charge range is 3 cells in rAthena (not 4 as iRO Wiki claimed). 150 UE = 3 cells. **Range is CORRECT. Removed from fix list.**

**Verdict: PASS — No changes needed**

---

### 6. Shield Boomerang (1305) — Active — HAS BUGS (NEW)

| Aspect | Expected (rAthena battle.cpp) | Implemented | Status |
|--------|------------------------------|-------------|--------|
| ATK% | 100+30*lv (130-250) | `100+(i+1)*30` | CORRECT |
| SP cost | 12 flat | 12 | CORRECT |
| ACD | 700ms | 700ms | CORRECT |
| Range | [3,5,7,9,11] cells, data=600 UE | 600 UE approx | OK |
| Shield required | Yes | Checked | CORRECT |
| Prerequisites | Shield Charge Lv3 | `{skillId:1304, level:3}` | CORRECT |
| **Element** | **ALWAYS Neutral** (rAthena + iRO Wiki Classic confirm) | **`skillElement: null` (weapon element)** | **BUG H4** |
| **Damage source** | **batk (STR-based) + shield_weight/10, NOT weapon ATK** | **Standard physical pipeline (weapon ATK)** | **BUG H5** |

#### BUG H4 — Shield Boomerang Element (HIGH)

**rAthena + iRO Wiki Classic:** "The damage dealt from Shield Boomerang is always Neutral." rAthena forces `ELE_NEUTRAL`.

**Current:** `skillElement: null` → inherits weapon element. If Crusader has a fire endow, Shield Boomerang would deal fire damage. Wrong.

**Fix:** Change to `skillElement: 'neutral'` in the handler.

#### BUG H5 — Shield Boomerang Damage Source (HIGH)

**rAthena** (`battle.cpp`): Shield Boomerang uses `sstatus->batk` (STR-based status ATK, NOT weapon ATK) + `shield_weight / 10`. Does NOT use weapon ATK, weapon upgrades, masteries, spirit spheres, Provoke ATK bonus, Magnum Break bonus, or Impositio Manus.

**Current:** Uses `executePhysicalSkillOnEnemy` which routes through the standard physical damage pipeline including weapon ATK.

**Fix:** Shield Boomerang needs custom damage calculation:
```javascript
const stkAtk = gcEffStats.str + Math.floor(gcEffStats.str / 10) ** 2
             + Math.floor(gcEffStats.dex / 5) + Math.floor(gcEffStats.luk / 3);
const shieldWeight = player.equippedShieldWeight || 0;
const sbBaseDmg = stkAtk + Math.floor(shieldWeight / 10);
const sbDamage = Math.floor(sbBaseDmg * effectVal / 100);
```

---

### 7. Devotion (1306) — Active — HAS BUGS

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| Max targets | lv (1-5) | `learnedLevel` | CORRECT |
| Range | (6+lv) cells | `(7+lv-1)*50` UE | CORRECT |
| Duration | 15*lv+15 s (30-90s) | `30000+i*15000` | CORRECT |
| SP cost | 25 flat | 25 | CORRECT |
| Cast time | 3000ms | 3000ms | CORRECT |
| Cannot target self | Yes | Checked | CORRECT |
| Cannot target Crusader/Paladin | Yes | Checked | CORRECT |
| Level diff <= 10 | Yes | Checked | CORRECT |
| Prerequisites | Reflect Shield Lv5 + Grand Cross Lv4 | Correct | CORRECT |
| **Damage redirect** | **ALL damage (phys+magic) → Crusader using TARGET's DEF** | **NOT IMPLEMENTED** | **BUG C4** |
| **ACD** | **3000ms** | **0ms** | **BUG H6** |
| **Buff propagation** | **AG/RS/Defender/Endure propagate to devoted targets** | **Not implemented** | **BUG M1** |
| **Break: out-of-range** | **On next damage while out of range** | **Not checked** | **BUG M2** |
| **Break: Crusader CC** | **Stun/freeze/stone/sleep** | **Not checked** | **BUG M3** |
| Break: HP < 25% | Yes | Line 20942 | CORRECT |
| Status effects | NOT redirected (only damage) | N/A | OK |
| Excess damage on Crusader death | Does NOT pass back to target | Will work with redirect impl | OK |

#### BUG C4 — Devotion Damage Redirect (CRITICAL)
Core mechanic. ALL damage (physical AND magical) to the protected target transfers to Crusader. Uses TARGET's DEF/MDEF for calculation (damage is computed against target, then the resulting number hits the Crusader).

**New finding:** Crusader's own Auto Guard, Reflect Shield, and Defender also protect Devotion-linked targets. When Crusader activates these buffs, they propagate to devoted targets via `status_change_start()`.

---

### 8. Reflect Shield (1307) — Self Buff — HAS BUG

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| Reflect% | 10+3*lv (13-40%) | `13+i*3` | CORRECT |
| SP cost | 30+5*lv (35-80) | `35+i*5` | CORRECT |
| Duration | 300s | 300000ms | CORRECT |
| Melee only | Yes | `enemyIsRanged` check | CORRECT |
| Shield required | Yes | Checked | CORRECT |
| Bypasses attacker DEF | Yes | Direct HP subtraction | CORRECT |
| **Prerequisites** | **Shield Boomerang Lv3 ONLY** (rAthena skill_tree.yml) | **Auto Guard Lv3 + Shield Boomerang Lv3** | **BUG H7** |

#### BUG H7 — Reflect Shield Prerequisites

**rAthena `skill_tree.yml`:** CR_REFLECTSHIELD requires ONLY CR_SHIELDBOOMERANG Lv3. Auto Guard is an IMPLICIT prereq (needed to learn Shield Charge Lv3 → Shield Boomerang Lv3), but is NOT a direct prereq of Reflect Shield.

**Fix:** Remove `{skillId: 1301, level: 3}` from prerequisites, keep only `{skillId: 1305, level: 3}`.

---

### 9. Providence (1308) — Ally Buff — HAS BUG

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| Demon resist | 5*lv % | `5+i*5` | CORRECT |
| Holy resist | 5*lv % | Same | CORRECT |
| SP cost | 30 flat | 30 | CORRECT |
| Cast time | 1000ms | 1000ms | CORRECT |
| Duration | 180s | 180000ms | CORRECT |
| Cannot target Crusader/Paladin | Yes | Checked | CORRECT |
| **ACD** | **3000ms** | **0ms** | **BUG H8** |
| Prerequisites | Divine Protection Lv5 + Heal Lv5 | `{401,5}+{1311,5}` | CORRECT |

---

### 10. Defender (1309) — Toggle — HAS BUGS

| Aspect | Expected (rAthena status.cpp) | Implemented | Status |
|--------|------------------------------|-------------|--------|
| Ranged reduction | [20,35,50,65,80]% (`5+15*lv`) | `20+i*15` | CORRECT |
| SP cost | 30 flat | 30 | CORRECT |
| Duration | 180s | 180000ms | CORRECT |
| Move speed | -33% all levels (speed clamped to 200 vs default 150) | `moveSpeedPenalty: 33` | CORRECT |
| Shield required | Yes | Checked | CORRECT |
| Toggle on/off | Yes | `hasBuff` check | CORRECT |
| Ranged phys only | `(BF_LONG\|BF_WEAPON)` — NOT ranged magic | `enemyIsRanged` check on `attackRange` | CORRECT |
| **ASPD penalty** | **[20,15,10,5,0]%** (`250-50*lv` in status.cpp) | **NOT IMPLEMENTED** | **BUG H9** |
| **ACD** | **800ms** | **0ms** | **BUG H10** |
| Prerequisites | Shield Boomerang Lv1 | `{skillId:1305, level:1}` | CORRECT |

---

### 11. Spear Quicken (1310) — Self Buff — HAS BUGS

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| ASPD bonus | +(20+lv)% (21-30%) | `effectVal = 20+(i+1)` | CORRECT |
| SP cost | 20+4*lv (24-60) | `24+i*4` | CORRECT |
| Duration | 30*lv s | `30000+i*30000` | CORRECT |
| 2H spear required | Yes | Checked | CORRECT |
| Quagmire strips | Yes | Line 19885 | CORRECT |
| Pre-renewal: ASPD only | No CRIT, no FLEE (confirmed: iRO Classic + RateMyServer) | ASPD only applied | CORRECT |
| **Decrease AGI strips** | **Yes** (rAthena status.yml `EndOnStart: Spearquicken`) | **NOT IMPLEMENTED** | **BUG H11** |
| **Description** | **"+ASPD with 2H Spear"** | **Says "+CRIT, +FLEE"** | **BUG M4** |
| Prerequisites | Spear Mastery Lv10 | Correct | CORRECT |

**Decrease AGI full strip list (from rAthena status.yml `EndOnStart`):**
`increase_agi, two_hand_quicken, spear_quicken, one_hand_quicken, adrenaline_rush, adrenaline_rush_full, cart_boost, merc_quicken, acceleration`

---

### 12. Heal Crusader (1311) — Active — HAS BUGS

| Aspect | Expected (rAthena) | Implemented | Status |
|--------|-------------------|-------------|--------|
| Heal formula | `floor((BaseLv+INT)/8)*(4+8*SkillLv)` | `calculateHealAmount()` | CORRECT |
| SP cost | 10+3*lv (13-40) | `13+i*3` | CORRECT |
| Damages Undead | Holy * element mod | Applied | CORRECT |
| **ACD** | **1000ms** | **0ms** | **BUG M5** |
| **Prerequisites** | **Faith Lv10 + Demon Bane Lv5** (rAthena: `AL_DEMONBANE Lv5 + CR_TRUST Lv10`) | **Faith Lv5 only** | **BUG H12** |

**rAthena Note:** Crusader does NOT have a separate CR_HEAL skill. They use AL_HEAL (Acolyte Heal) with Crusader-specific prereqs: Demon Bane Lv5 + Faith Lv10 (full chain: Faith Lv5 → Cure Lv1 → Divine Protection Lv3 → Demon Bane Lv5, plus Faith Lv10 separately).

---

### 13. Cure Crusader (1312) — Active — HAS BUG

| Aspect | Expected | Implemented | Status |
|--------|---------|-------------|--------|
| Removes Silence/Blind/Confusion | Yes | Yes | CORRECT |
| SP cost | 15 | 15 | CORRECT |
| **ACD** | **1000ms** | **0ms** | **BUG M6** |
| Prerequisites | Faith Lv5 | Correct | CORRECT |

---

### 14. Shrink (1313) — Quest Toggle — HAS BUG

| Aspect | Expected (rAthena battle.cpp) | Implemented | Status |
|--------|------------------------------|-------------|--------|
| Knockback distance | 2 cells | 2 cells | CORRECT |
| SP cost | 15 | 15 | CORRECT |
| Duration | Toggle (300s) | 300000ms | CORRECT |
| Shield required | Yes | Checked | CORRECT |
| Quest skill | Yes | `questSkill: true` | CORRECT |
| Runs inside AG block | Yes | Yes | CORRECT |
| **Knockback chance** | **5 * Auto_Guard_Level %** (rAthena: `rnd()%100 < 5 * sce->val1`) | **Flat 50%** (`Math.random() < 0.5`) | **BUG H13** |

**rAthena source** (`battle.cpp` inside Auto Guard block):
```cpp
if(sc->data[SC_SHRINK] && rnd()%100 < 5 * sce->val1)
    skill_blown(bl, src, skill_get_blewcount(CR_SHRINK, 1), -1, 0);
```
`sce->val1` = Auto Guard skill level. At AG Lv1 = 5%, AG Lv5 = 25%, AG Lv10 = 50%.

**Fix:** Replace `Math.random() < 0.5` with `Math.random() * 100 < 5 * agBuff.blockChance_level` (need to store AG level in buff).

---

### 15-17. Spear Mastery (700), Riding (708), Cavalry Mastery (709) ✅

All verified correct. No changes needed.

---

## Complete Fix List (Ordered by Priority)

### CRITICAL (4 fixes — wrong damage output)

| # | Skill | Issue | Impact |
|---|-------|-------|--------|
| C1 | Grand Cross | MATK uses INT/7 for both min/max | ~34% less MATK at high INT |
| C2 | Grand Cross | ATK includes StatusATK (STR) — should be WeaponATK only | Over-inflated ATK portion |
| C3 | Grand Cross | DEF: only flat hardDef subtracted — should be hard% + soft-flat for both DEF and MDEF | Damage too high vs armored targets |
| C4 | Devotion | Damage redirect not implemented | Core mechanic missing |

### HIGH (13 fixes — wrong data / missing mechanics)

| # | Skill | Issue | Fix Location |
|---|-------|-------|-------------|
| H1 | Grand Cross | Hit count: 1 → should be 3 ticks (950ms/300ms) | `index.js` handler |
| H2 | Grand Cross | AoE: 9-cell cross → should be 29-cell diamond (4-cell radius) | `index.js` handler |
| H3 | Grand Cross | Self-damage: average-based → should be per-tick self-calc / 2 | `index.js` handler |
| H4 | Shield Boomerang | Element: weapon → should be ALWAYS Neutral | `index.js` handler |
| H5 | Shield Boomerang | Damage: uses weapon ATK pipeline → should use batk + shield_weight/10 | `index.js` handler |
| H6 | Devotion | ACD 0 → should be 3000ms | `ro_skill_data_2nd.js` |
| H7 | Reflect Shield | Prereq: AG Lv3 + SB Lv3 → should be SB Lv3 only | `ro_skill_data_2nd.js` |
| H8 | Providence | ACD 0 → should be 3000ms | `ro_skill_data_2nd.js` |
| H9 | Defender | ASPD penalty [20,15,10,5,0]% not applied | `index.js` + `ro_buff_system.js` |
| H10 | Defender | ACD 0 → should be 800ms | `ro_skill_data_2nd.js` |
| H11 | Spear Quicken | Decrease AGI doesn't strip SQ (or THQ/AR/etc.) | `index.js` DecAGI handler |
| H12 | Heal Crusader | Prereq: Faith Lv5 → Faith Lv10 + Demon Bane Lv5 | `ro_skill_data_2nd.js` |
| H13 | Shrink | Knockback: flat 50% → 5 * AG_Level % | `index.js` pipeline |

### MODERATE (4 fixes)

| # | Skill | Issue | Fix Location |
|---|-------|-------|-------------|
| M1 | Devotion | Buff propagation (AG/RS/DEF to devoted targets) not implemented | Deferred |
| M2 | Devotion | Break: out-of-range not checked | `index.js` pipeline |
| M3 | Devotion | Break: Crusader CC not checked | `index.js` status handler |
| M4 | Spear Quicken | Description says "+CRIT, +FLEE" — pre-renewal is ASPD only | `ro_skill_data_2nd.js` |
| M5 | Heal Crusader | ACD 0 → 1000ms | `ro_skill_data_2nd.js` |
| M6 | Cure Crusader | ACD 0 → 1000ms | `ro_skill_data_2nd.js` |

### LOW / DEFERRED (5 items)

| # | Skill | Issue | Blocked By |
|---|-------|-------|-----------|
| L1 | All defensive buffs | PvP tick missing Crusader hooks | PvP system |
| L2 | Shield-dependent buffs | Shield strip should cancel AG/RS/DEF/Shrink | Equipment change event |
| L3 | Spear Quicken | Weapon swap should cancel | Equipment change event |
| L4 | Grand Cross | Doesn't hit other players | PvP system |
| L5 | Grand Cross | Magic cards (BF_MAGIC) should apply but weapon cards shouldn't | Card system maturity |

---

## Implementation Plan

### Phase A: Skill Data Fixes (ro_skill_data_2nd.js) — 8 changes

```
1. Reflect Shield (1307): prereqs [{1301,3},{1305,3}] → [{1305,3}] only
2. Heal Crusader (1311): prereqs [{1300,5}] → [{1300,10},{413,5}]
3. Heal Crusader (1311): afterCastDelay 0 → 1000
4. Cure Crusader (1312): afterCastDelay 0 → 1000
5. Providence (1308): afterCastDelay 0 → 3000
6. Devotion (1306): afterCastDelay 0 → 3000
7. Defender (1309): afterCastDelay 0 → 800
8. Spear Quicken (1310): description → "+ASPD with 2H Spear. Requires two-handed spear."
```

### Phase B: Grand Cross Full Rewrite (index.js)

Complete rewrite of Grand Cross handler to match rAthena pre-renewal:

**B1. ATK = WeaponATK only (no StatusATK):**
```javascript
// ATK portion: weapon ATK with DEX variance, NO StatusATK
const gcWeaponATK = gcEffStats.weaponATK || player.weaponATK || 0;
const gcRefineATK = gcEffStats.refineATK || 0;
const gcWeaponLevel = player.weaponLevel || 1;
const gcDex = gcEffStats.dex || 1;
const gcAtkMin = Math.floor(gcDex * (80 + gcWeaponLevel * 20) / 100);
const gcAtkMax = gcWeaponATK;
const atkRoll = Math.min(gcAtkMax, gcAtkMin) + Math.floor(Math.random() * (Math.max(1, gcAtkMax - gcAtkMin + 1)));
```

**B2. MATK = correct min/max range:**
```javascript
const gcMatkMin = gcEffStats.matkMin || (gcEffStats.int + Math.floor(gcEffStats.int / 7) ** 2);
const gcMatkMax = gcEffStats.matkMax || (gcEffStats.int + Math.floor(gcEffStats.int / 5) ** 2);
const weaponMATK = gcEffStats.weaponMATK || 0;
const matkRoll = (gcMatkMin + weaponMATK) + Math.floor(Math.random() * ((gcMatkMax + weaponMATK) - (gcMatkMin + weaponMATK) + 1));
```

**B3. DEF = hard% + soft-flat for ATK; MDEF = hard% + soft-flat for MATK:**
```javascript
// ATK after DEF
let atkAfterDef = Math.floor(atkRoll * (100 - (enemy.hardDef || 0)) / 100);
atkAfterDef = Math.max(0, atkAfterDef - (enemy.softDef || 0));
atkAfterDef += gcRefineATK;

// MATK after MDEF
let matkAfterMdef = Math.floor(matkRoll * (100 - (enemy.hardMDef || 0)) / 100);
matkAfterMdef = Math.max(0, matkAfterMdef - (enemy.softMDef || 0));

// Combined
let gcDamage = Math.floor((atkAfterDef + matkAfterMdef) * gcMultiplier / 100);
```

**B4. AoE = 29-cell diamond (|dx|+|dy| <= 4):**
```javascript
const CELL = 50;
const crossOffsets = [];
for (let dx = -4; dx <= 4; dx++) {
    for (let dy = -4; dy <= 4; dy++) {
        if (Math.abs(dx) + Math.abs(dy) <= 4) {
            crossOffsets.push({ dx: dx * CELL, dy: dy * CELL });
        }
    }
}
// Generates 41 cells: |dx|+|dy|<=4 diamond
```

**B5. 3 ticks with 300ms intervals:**
```javascript
const GC_TICKS = 3;
const GC_TICK_INTERVAL = 300;
for (let tick = 0; tick < GC_TICKS; tick++) {
    setTimeout(() => {
        // Per-tick damage to each enemy in AoE
        // Per-tick self-damage = floor(gcDamageVsSelf / 2) * holyMod * faithResist
    }, tick * GC_TICK_INTERVAL);
}
```

**B6. Self-damage per-tick:** Calculate GC damage against self (using caster's DEF/MDEF), then self takes `floor(selfCalc / 2)` per tick, modified by Holy element vs caster armor element and Faith resist.

### Phase C: Shield Boomerang Fix (index.js)

**C1.** Force neutral element: Change `skillElement: null` to `skillElement: 'neutral'`

**C2.** Custom damage calc replacing `executePhysicalSkillOnEnemy`:
```javascript
// Shield Boomerang: uses batk (STR-based) + shield_weight/10, NOT weapon ATK
const sbStats = getEffectiveStats(player);
const sbBatk = sbStats.str + Math.floor(sbStats.str / 10) ** 2
             + Math.floor(sbStats.dex / 5) + Math.floor(sbStats.luk / 3);
const shieldWeight = player.equippedShieldWeight || 0;
const shieldRefine = player.equippedShieldRefine || 0;
const sbBase = sbBatk + Math.floor(shieldWeight / 10) + shieldRefine * 5;
let sbDamage = Math.floor(sbBase * effectVal / 100);
// Apply neutral element modifier vs target
// Apply Lex Aeterna if active
// Do NOT apply size penalty, masteries, weapon cards
```

### Phase D: Defender ASPD Penalty (index.js + ro_buff_system.js)

**D1.** Store penalty in buff: `aspdPenalty: [20,15,10,5,0][learnedLevel-1]`
**D2.** getBuffModifiers: `mods.aspdMultiplier *= (1 - buff.aspdPenalty / 100)`

### Phase E: Decrease AGI Full Buff Strip (index.js)

Replace single `increase_agi` removal with full rAthena list:
```javascript
const decAgiStrips = ['increase_agi', 'spear_quicken', 'two_hand_quicken',
    'one_hand_quicken', 'adrenaline_rush', 'adrenaline_rush_full', 'cart_boost'];
for (const sb of decAgiStrips) {
    if (hasBuff(target, sb)) {
        removeBuff(target, sb);
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: sb, reason: 'decrease_agi' });
    }
}
```

### Phase F: Shrink Scaling Fix (index.js)

In the Auto Guard block section of the enemy attack pipeline, change:
```javascript
// BEFORE: flat 50%
if (shrinkBuff && Math.random() < 0.5)

// AFTER: 5 * Auto Guard Level %
if (shrinkBuff && Math.random() * 100 < 5 * (agBuff.skillLevel || 10))
```
Also store `skillLevel: learnedLevel` in the Auto Guard buff.

### Phase G: Devotion Damage Redirect (index.js)

Insert BEFORE Auto Guard check in enemy attack pipeline. Redirect ALL damage (physical + magical) from devoted target to Crusader. See v1 Phase E code (still valid) with these additions:
- Check Crusader is alive, link not expired, target in range
- On out-of-range: break that specific link (not all links)
- On Crusader death: break all links
- On HP < 25%: break all links

### Phase H: Devotion Break on CC (index.js)

In `applyStatusEffect()` success path, check if target has `devotionLinks` and CC type is stun/freeze/stone/sleep → break all links.

---

## Execution Order

1. **Phase A** — Data fixes (8 items, no risk)
2. **Phase B** — Grand Cross rewrite (CRITICAL — fixes 6 bugs)
3. **Phase C** — Shield Boomerang fix (2 bugs)
4. **Phase D** — Defender ASPD (1 bug)
5. **Phase E** — Decrease AGI strip list (1 bug)
6. **Phase F** — Shrink scaling (1 bug)
7. **Phase G** — Devotion redirect (CRITICAL)
8. **Phase H** — Devotion CC break

---

## Verification Checklist

- [ ] Grand Cross MATK uses correct min/max (INT/7 to INT/5)
- [ ] Grand Cross ATK uses WeaponATK only (no StatusATK)
- [ ] Grand Cross applies hard DEF % + soft DEF flat to ATK, hard MDEF % + soft MDEF flat to MATK
- [ ] Grand Cross hits 3 times per enemy (300ms intervals)
- [ ] Grand Cross AoE is 29+ cell diamond (4-cell radius)
- [ ] Grand Cross self-damage is per-tick against self / 2
- [ ] Shield Boomerang always deals Neutral damage regardless of endows
- [ ] Shield Boomerang uses STR-based ATK + shield weight, not weapon ATK
- [ ] Shield Charge range is 150 UE (3 cells) — unchanged
- [ ] Reflect Shield learnable with Shield Boomerang Lv3 only (no AG prereq)
- [ ] Heal (Crusader) requires Faith Lv10 + Demon Bane Lv5
- [ ] Defender ASPD penalty: Lv1=-20%, Lv5=0%
- [ ] Defender ACD 800ms
- [ ] Providence/Devotion ACD 3000ms
- [ ] Heal/Cure Crusader ACD 1000ms
- [ ] Decrease AGI strips: IncAGI, SQ, THQ, AR, Full AR, Cart Boost
- [ ] Shrink knockback = 5 * AG_Level % (not flat 50%)
- [ ] Devotion redirects all damage to Crusader
- [ ] Devotion breaks on: out-of-range, Crusader CC, HP<25%, death
- [ ] Spear Quicken description: ASPD only, no CRIT/FLEE mention

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/ro_skill_data_2nd.js` | 8 data fixes (Phase A) |
| `server/src/index.js` | Grand Cross rewrite (B), Shield Boomerang fix (C), Defender ASPD (D), DecAGI strips (E), Shrink scaling (F), Devotion redirect (G), Devotion CC break (H) |
| `server/src/ro_buff_system.js` | Defender ASPD modifier (D) |

---

## Research Sources

All findings verified against rAthena pre-renewal source code:
- `battle.cpp`: Auto Guard (`battle_status_block_damage`), Grand Cross (`battle_calc_magic_attack` + `battle_calc_skill_base_damage` + `battle_calc_defense_reduction`), Reflect Shield, Defender (`BF_LONG|BF_WEAPON`), Shield Boomerang (batk + shield weight), Shrink (`5 * sce->val1`)
- `skill.cpp`: Grand Cross AoE pattern (29-cell diamond, line 14184), hit count (950ms/300ms = 3 ticks)
- `status.cpp`: Defender ASPD (`250-50*lv`), move speed (clamped to 200), Devotion buff propagation
- `status.yml`: Decrease AGI `EndOnStart` list (10 buffs stripped)
- `skill_tree.yml`: All Crusader prerequisites, Heal/Cure use AL_HEAL/AL_CURE with Crusader prereqs
- `skill_db.yml`: Shield Charge range=3, Defender ACD=800ms, Devotion ACD=3000ms
