# Second-Pass Review: Combat Systems

**Reviewer**: Claude Code (Second-Pass Cross-Verification)
**Date**: 2026-03-23
**Scope**: Cross-verification of AUDIT_01 (Stats/Leveling), AUDIT_02 (Combat/Damage), AUDIT_03 (Element/Size/Race) against actual server code
**Files Verified**: `ro_damage_formulas.js` (full), `index.js` (targeted searches at ~30 locations)

---

## Verified Discrepancies (confirmed real with line numbers)

### VD-1: StatusATK Missing `floor(BaseLv/4)` -- CONFIRMED CRITICAL
- **Audit Source**: AUDIT_01 C1-1, AUDIT_02 P1
- **Code Location**: `ro_damage_formulas.js` lines 288-290
- **Verified Code**:
  ```javascript
  // Melee:  STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/5)
  const statusATK = isRangedWeapon
      ? dex + Math.floor(dex / 10) ** 2 + Math.floor(str / 5) + Math.floor(luk / 5)
      : str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 5);
  ```
- **Research Docs**: `04_Physical_Damage.md` line 80: `StatusATK = floor(BaseLv / 4) + STR + floor(STR / 10)^2 + floor(DEX / 5) + floor(LUK / 3)`
- **Verdict**: REAL. The `floor(BaseLv/4)` term is completely absent. At level 99 this is -24 StatusATK on every physical attack. The `level` variable IS available in the function scope (line 269).

### VD-2: StatusATK Uses `LUK/5` Instead of `LUK/3` -- CONFIRMED CRITICAL
- **Audit Source**: AUDIT_01 A6-3, AUDIT_02 P2
- **Code Location**: `ro_damage_formulas.js` lines 289-290 (`Math.floor(luk / 5)` appears twice)
- **Research Docs**: `04_Physical_Damage.md` line 80 confirms `floor(LUK / 3)`, `01_Stats_Attributes.md` line 103 also uses `LUK/3`
- **Verdict**: REAL. At 99 LUK: server gives 19, correct gives 33. Combined with VD-1, a level 99 character with 99 LUK is missing 38 StatusATK.

### VD-3: Critical Hits Do Not Bypass DEF -- CONFIRMED CRITICAL
- **Audit Source**: AUDIT_02 P4
- **Code Location**: `ro_damage_formulas.js` lines 748-779
- **Verified**: Line 749: `let skipDEF = ignoreDefense || false;` -- `isCritical` is never checked. The crit bonus at line 617-622 is applied pre-DEF. Then lines 769-779 apply full Hard DEF + Soft DEF reduction to critical hits.
- **Research Docs**: `04_Physical_Damage.md` lines 954-958 explicitly say "Skip if: critical hit" for both Hard and Soft DEF. Line 1000: "criticals skip Steps 14 and 15 entirely".
- **Index.js verification**: Only one place uses `ignoreDefense: true` with `forceCrit: true` together (line 30654, which is Auto Counter). Normal auto-attack crits at line 25046 pass no `ignoreDefense` flag.
- **Verdict**: REAL. This is the single most impactful combat formula bug. Critical builds against high-DEF targets get the +40% bonus applied to pre-DEF damage, then DEF reduces it, making crits far weaker than intended.

### VD-4: Monster Soft MDEF Uses Player Formula -- CONFIRMED CRITICAL
- **Audit Source**: AUDIT_03 CRIT-2
- **Code Location**: `index.js` line 4466
- **Verified Code**: `softMDef: Math.floor((template.stats?.int || 0) + Math.floor((template.stats?.vit || 0) / 5) + Math.floor((template.stats?.dex || 0) / 5) + Math.floor((template.level || 1) / 4))`
- **Correct Formula (from `08_Size_Race_MonsterProps.md` line 434)**: `MonsterSoftMDEF = floor((MonsterINT + MonsterLevel) / 4)`
- **Example**: INT 50, Level 60, VIT 40, DEX 30 monster: Server gives 50+8+6+15=79. Correct: floor(110/4)=27.
- **Duplicate**: Also at line 29689 (metamorphosis respawn path) -- same wrong formula.
- **Also wrong at lines 16024 and 16061-16062** (Abracadabra summon/metamorphosis): These use even simpler wrong formulas.
- **Verdict**: REAL. Magic damage against monsters is massively underperforming because soft MDEF is 2-3x higher than it should be.

### VD-5: Monster Soft DEF Uses Player Formula -- CONFIRMED HIGH
- **Audit Source**: AUDIT_03 HIGH-1
- **Code Location**: `index.js` line 4465
- **Verified Code**: `softDef: Math.floor(((template.stats?.vit || 0)) * 0.5 + Math.max(0, (template.stats?.vit || 0) - 20) * 0.3)`
- **Correct Formula**: `floor(VIT / 2)` (monster-specific, from `08_Size_Race_MonsterProps.md`)
- **Example**: VIT 50 monster: Server gives 25+9=34. Correct: 25.
- **Duplicate**: Also at line 29688 (metamorphosis respawn path).
- **Verdict**: REAL. Physical damage against high-VIT monsters is slightly reduced beyond intended.

### VD-6: `modeFlags.isBoss` Never Set -- CONFIRMED CRITICAL
- **Audit Source**: AUDIT_03 CRIT-1
- **Code Location**: `parseModeFlags()` at `index.js` lines 411-432 -- no `isBoss` field. Boss protocol at lines 4427-4431 sets `knockbackImmune`, `statusImmune`, `detector`, `mvp` but NOT `isBoss`.
- **Impact Locations**: `ro_damage_formulas.js` line 672 (`target.modeFlags.isBoss`), line 831 (`attacker.modeFlags.isBoss`), line 996 (magic path `target.modeFlags.isBoss`), `index.js` line 3389 (knockback).
- **Verified**: Searched all `isBoss` references -- numerous places in `index.js` check `modeFlags.isBoss` (lines 13598, 15428, etc.) and it's always undefined/falsy.
- **Fallback check**: Most boss immunity checks in skill handlers use `enemy.monsterClass === 'boss' || enemy.monsterClass === 'mvp'` or `modeFlags.statusImmune` as alternatives -- those DO work. But the damage pipeline card bonus checks in `ro_damage_formulas.js` exclusively use `modeFlags.isBoss`.
- **Verdict**: REAL. Abysmal Knight Card (+25% vs boss), Alice Card (-40% from boss), and Vesper Card (ignore boss MDEF) are all broken because boss/normal class detection fails in the damage formulas.

### VD-7: Negative Element Modifier Treated as Immune -- CONFIRMED HIGH
- **Audit Source**: AUDIT_03 HIGH-3
- **Code Location**: `ro_damage_formulas.js` lines 720-726 (physical), lines 982-986 (magic)
- **Verified**: When `eleModifier <= 0`, both paths set `damage: 0` and return. For negative values, `hitType: 'elementHeal'` is set but no HP restoration logic exists anywhere.
- **Verdict**: REAL. Poison/Shadow attacks vs Undead should heal but instead do 0 damage. Same for same-element attacks at high levels.

### VD-8: ASPD Attack Delay Uses `*50` Instead of `*20` -- CONFIRMED (Intentional?)
- **Audit Source**: AUDIT_01 C5-2, AUDIT_02 P6
- **Code Location**: `index.js` line 460: `interval = (200 - aspd) * 50;`
- **Research Docs**: `06_ASPD_Hit_Flee_Critical.md` line 249 says `*20`. Line 891 of the same doc acknowledges the `*50` exists and questions if intentional.
- **Note**: The ASPD cap is also 195 (not 190), and there's a custom diminishing returns system above 195 (lines 462-467). This appears to be a deliberate game design choice -- a slower, more strategic combat pacing. ASPD 195 * 50 = 250ms vs the RO spec of ASPD 190 * 20 = 200ms. The difference is small at cap but large at low ASPD values.
- **Verdict**: REAL discrepancy from RO Classic, but likely INTENTIONAL DESIGN. Should be documented clearly, not "fixed."

### VD-9: Provoke atkMultiplier Applied to Magic Damage -- CONFIRMED MEDIUM
- **Audit Source**: AUDIT_02 P8
- **Code Location**: `ro_damage_formulas.js` lines 960-961 in `calculateMagicalDamage()`
- **Verified**: `const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;` then `totalDamage = Math.floor(totalDamage * atkMultiplier);`
- **Provoke**: Sets `atkMultiplier: 1.32` at max level. This incorrectly boosts magic damage by 32%.
- **Verdict**: REAL. Provoke should only affect physical ATK in RO Classic.

### VD-10: Magic Damage Missing Defensive Card Reductions -- CONFIRMED HIGH
- **Audit Source**: AUDIT_03 MED-5
- **Code Location**: `ro_damage_formulas.js` `calculateMagicalDamage()` lines 919-1054
- **Verified**: The physical path has Step 8c (lines 818-825) with `target.cardDefMods` race/ele/size reductions. The magic path has NO equivalent block. Searched the entire `calculateMagicalDamage()` function -- no `cardDefMods`, `cardSubClass`, or `cardLongAtkDef` references.
- **Verdict**: REAL. Thara Frog Card (-30% vs Demi-Human), Raydric Card (-20% vs Neutral), etc. do not reduce magic damage. This makes shield cards much weaker for PvP than intended.

### VD-11: Freeze/Stone Element Override Missing from Physical Damage Path -- CONFIRMED MEDIUM
- **Audit Source**: AUDIT_03 MED-3
- **Code Location**: `getEnemyTargetInfo()` at `index.js` line 3904-3912
- **Verified**: `getEnemyTargetInfo()` reads `enemy.element` directly (line 3905). The `overrideElement` from freeze/stone status is available through `getCombinedModifiers(enemy)` (line 3909 sets buffMods), but it's NOT applied to the `element` field.
- **Magic paths**: 7 locations apply `if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;` (lines 10172, 10333, 10512, 10618, 10755, 10830, 10955).
- **Physical auto-attack path**: Line 25050 calls `getEnemyTargetInfo(enemy)` which returns the ORIGINAL element, not the frozen Water Lv1 override.
- **Verdict**: REAL. Physical attacks against frozen enemies use the original element instead of Water Lv1. Wind physical attacks against frozen targets miss the expected 175% bonus.

---

## False Positives (audit was wrong, code is actually correct)

### FP-1: "SP Regen Bonus at INT >= 120 Not Implemented" -- AUDIT_01 WAS WRONG
- **Audit Claim**: AUDIT_01 Section A4 listed "SP Regen bonus (INT >= 120): floor(INT/2) - 56 additional -- Not found in server code" as MISSING.
- **Actual Code**: `index.js` lines 26662-26663:
  ```javascript
  if (intStat >= 120) {
      spRegen += 4 + Math.floor(intStat / 2 - 60);
  }
  ```
- **Verdict**: IMPLEMENTED. The audit missed it. (The formula `4 + floor(INT/2 - 60)` is equivalent to `floor(INT/2) - 56` for integer inputs.)

### FP-2: "SoftDEF Formula Is Wrong" -- AUDIT_02 OVERSTATED THE ISSUE
- **Audit 02 Claim**: P3 says the softDEF formula `VIT/2 + AGI/5 + BaseLv/2` is "completely wrong" and should be `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)`.
- **Deep Research Reality**: `01_Stats_Attributes.md` line 522 gives the FULL formula: `softDEF = vit_def_base + vit_def_random + floor(AGI / 5) + floor(BaseLevel / 2)` where `vit_def_base = floor(VIT * 0.5)`.
- **The server uses the DISPLAY formula** (line 527): `DisplaySoftDEF = floor(VIT/2) + floor(AGI/5) + floor(BaseLevel/2)` -- which is the deterministic base WITHOUT the per-hit random VIT component.
- **What's ACTUALLY missing**: Only the per-hit random VIT component (`vit_def_random`). The AGI and BaseLv terms ARE correct and ARE part of the official formula. Audit 02 was wrong to say "AGI contributes to SoftDEF in the implementation but not in RO Classic" -- AGI IS in the official formula.
- **Audit 02 also incorrectly claims the formula should be `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)`** -- this omits the AGI and BaseLv terms entirely and is actually LESS accurate than what the server has.
- **Verdict**: The server's base formula is CORRECT. Only the random VIT component is missing. This is a LOW-MEDIUM issue (adds variance, not a systematic over/undershoot). Audit 02 P3 severity should be downgraded from CRITICAL to LOW-MEDIUM.

### FP-3: "Hard DEF Uses Simplified Linear Formula" -- AUDIT_02 SEVERITY OVERSTATED
- **Audit 02 Claim**: P7 says the server uses `(100 - DEF) / 100` and should use `(4000 + DEF) / (4000 + DEF * 10)`.
- **Context**: The code at line 776 uses `Math.floor(totalATK * (100 - hardDef) / 100)` with hardDef capped at 99. Audit 02 correctly notes this is the rAthena simplified formula.
- **Reality**: Both formulas are used in various RO implementations. The simplified formula IS rAthena's actual code for pre-renewal. The `4000+DEF` formula is the iRO Wiki "official" formula. The deep research doc recommends the `4000+DEF` formula but acknowledges rAthena uses the simplified one.
- **HOWEVER**: Monster hard DEF values in the 509-template database are calibrated for the rAthena simplified formula (DEF 0-99 range). Switching to the `4000+DEF` formula without recalibrating monster DEF values would make monsters significantly easier to kill. This is a design decision, not a bug.
- **Verdict**: Not a bug per se -- it's a valid implementation choice. Both audits correctly identify the difference, but Audit 02 overstates the severity. This should be a DESIGN DECISION, not a priority fix.

### FP-4: "PD Applies to Skills" -- AUDIT_02 PARTIALLY WRONG
- **Audit 02 Claim**: P5 says PD applies to skills and should not.
- **Verified Code**: Line 485: `if (!forceHit && !forceCrit)` gates the PD check. ALL skill calls use `calculateSkillDamage()` (line 1814) which passes `{ isSkill: true, ...skillOptions }`. However, the PD check at line 485 does NOT check `isSkill` -- it only checks `forceHit` and `forceCrit`.
- **BUT**: Most physical skills in the server pass `forceHit: true` in their options (Bash, Magnum Break, etc.). Only skills that omit `forceHit` could be PD'd.
- **Verdict**: PARTIALLY REAL. The code is missing `!isSkill` in the PD guard, but the practical impact is reduced because most skills already pass `forceHit: true`. Still should be fixed for correctness.

### FP-5: "HIT/FLEE Base Constants 175/100" -- AUDIT_01 CONFUSED ABOUT THIS
- **Audit 01 Claim**: C4-1 and C4-2 say server adds +175 base HIT and +100 base FLEE as constants, while research says `BaseLv + DEX` and `BaseLv + AGI` with no base constant.
- **Deep Research Reality**: `06_ASPD_Hit_Flee_Critical.md` line 694 gives `Player HIT = 175 + BaseLv + DEX + BonusHIT` and line 705 gives `Player FLEE = 100 + BaseLv + AGI + BonusFLEE` -- the constants ARE part of the formula.
- **Verdict**: FALSE POSITIVE. The server matches the deep research formula exactly. Audit 01 was comparing against a simplified formula that omitted the base constants.

### FP-6: "Mystical Amplification (HW_MAGICPOWER) Missing" -- NOT A BUG
- **Audit 02 Claim**: M6 says Mystical Amplification is missing.
- **Reality**: Mystical Amplification (HW_MAGICPOWER) is a High Wizard skill (transcendent class). In this project, skill ID 811 is Frost Nova (base Wizard skill). The transcendent system is not implemented. This is an expected missing feature, not a bug in the existing system.
- **Verdict**: FALSE POSITIVE as a "combat damage bug." It's correctly categorized as part of the missing transcendent system.

---

## Newly Discovered Issues (missed by first audit)

### ND-1: Physical Damage `overrideElement` Not Applied in Auto-Attack Target Info
- **Location**: `getEnemyTargetInfo()` at `index.js` line 3905
- **Issue**: The function returns `element: enemy.element` which is the monster's base element. It also returns `buffMods: getBuffStatModifiers(enemy)` at line 3909 which DOES contain `overrideElement`. However, `calculatePhysicalDamage()` in `ro_damage_formulas.js` reads `target.element` (line 473) for the element table lookup, NOT `target.buffMods.overrideElement`.
- **This means**: Even though magic paths manually apply `overrideElement` to `targetInfo.element` (7 locations), the physical path and auto-attack path NEVER do this.
- **Impact**: Frozen enemies should take 175% from Wind physical attacks (Water Lv1 element). Currently they take damage based on their original element.
- **Fix**: In `getEnemyTargetInfo()`, check `buffMods.overrideElement` and override the returned `element` field.

### ND-2: Monster Soft DEF/MDEF Wrong in Abracadabra Summon Paths
- **Location**: `index.js` lines 16024, 16061-16062
- **Issue**: Abracadabra summon (line 16024) uses `softDef: tmpl.vit || 0` (just raw VIT, not even `VIT/2`). Metamorphosis (line 16061) uses `softDef: newTmpl.vit || 0` and `softMDef: Math.floor((newTmpl.int || 0) + (newTmpl.vit || 0) / 5 + (newTmpl.dex || 0) / 5)`. Neither uses the correct monster formulas.
- **This is a THIRD variant** of the monster softDef calculation, different from both the main spawn (line 4465, player formula) and the correct monster formula (`VIT/2`).
- **Impact**: Abracadabra-summoned monsters have wildly wrong soft defenses.

### ND-3: `calculateSkillDamage` Wrapper Argument Mismatch
- **Location**: `index.js` line 1814-1821
- **Issue**: The old `calculateSkillDamage()` function takes 9 positional arguments and remaps them into the new 3-argument `calculatePhysicalDamage()` wrapper. But the remapping at lines 1815-1820 passes `attackerStats` as argument 1 and `targetStats` as argument 2, while `calculatePhysicalDamage()` at line 2578 expects them in the SAME order. This works but is fragile.
- **More importantly**: `calculateSkillDamage()` passes `targetHardDef` as a separate argument (arg 3), but the wrapper puts it into `targetInfo` incorrectly -- it's passed as the 3rd positional arg to `calculatePhysicalDamage(attackerStats, targetStats, targetHardDef, ...)` which correctly assigns it. So this is actually fine structurally but very hard to maintain.
- **Verdict**: Code smell, not a bug. But worth noting since 50+ call sites depend on exact argument ordering.

### ND-4: `calculateMagicSkillDamage` Passes No Card Bonuses
- **Location**: `index.js` line 1824-1833
- **Issue**: The `calculateMagicSkillDamage()` helper passes `weaponMATK: 0` and `buffMods: { atkMultiplier: 1.0 }` without forwarding the caster's card bonuses (`cardMatkRate`, `cardMagicRace`, `cardSkillAtk`). These bonuses would only be applied if the caller manually adds them to the `attackerStats` or passes them separately.
- **Check**: Most magic skill handlers in `index.js` (e.g., bolt skills) call `calculateMagicSkillDamage()` with just stats -- they DON'T pass card MATK bonuses. The card bonuses are applied at a higher level in some handlers (via `buildMagicAttackerInfo`) but many direct calls miss them.
- **Impact**: Varies by skill. Some magic skills may not benefit from weapon MATK cards. Needs per-handler audit.

### ND-5: Crit Bonus Applied Before DEF Makes Crits Even Weaker
- **Location**: `ro_damage_formulas.js` lines 617-622 (crit bonus) vs 769-779 (DEF)
- **Issue**: The +40% crit bonus is applied at line 618 (Step 4 in code order), BEFORE DEF reduction at line 769 (Step 8). In RO Classic, since crits bypass DEF entirely, the order doesn't matter. But since this server does NOT bypass DEF for crits (VD-3), the +40% bonus is partially absorbed by DEF. If VD-3 is fixed (crits bypass DEF), this ordering issue goes away automatically.
- **Verdict**: Not a separate issue -- it's a consequence of VD-3.

---

## Cross-System Issues

### CS-1: Stat System -> Damage System: StatusATK Feeds Wrong Values
- **Chain**: `getEffectiveStats()` (index.js:3781) -> `calculateDerivedStats()` (ro_damage_formulas.js:267) -> `statusATK` (line 288-290) -> used in `calculatePhysicalDamage()` (line 551)
- **Issue**: The missing `BaseLv/4` and wrong `LUK/5` (VD-1, VD-2) propagate through the ENTIRE physical damage pipeline. Every physical attack (auto-attack AND skill) is affected because ALL paths go through `calculateDerivedStats()`.
- **Additionally**: The `statusATK` value is sent to the client for display (via `buildFullStatsPayload()`) -- the stat window also shows wrong ATK values.

### CS-2: Element Override System Inconsistency
- **Chain**: `applyStatusEffect()` -> `ro_status_effects.js` (freeze sets `overrideElement: { type: 'water', level: 1 }`) -> `getCombinedModifiers()` -> `buffMods.overrideElement`
- **Issue**: The override flows correctly into `buffMods` but is only consumed by MAGIC damage paths (7 locations). Physical auto-attacks and physical skills DO NOT apply the override because `getEnemyTargetInfo()` reads `enemy.element` directly.
- **This creates a bizarre situation**: A frozen Water-element monster takes 100% from Fire magic (Water Lv1 override applied) but takes 150% from Fire physical (original Water element used). The magic path is correct; the physical path is wrong.

### CS-3: Buff System -> Damage Pipeline: Boss Card Classification Broken
- **Chain**: Monster spawns -> `parseModeFlags()` (no `isBoss`) -> `enemy.modeFlags` stored -> `getEnemyTargetInfo()` -> `modeFlags` passed to damage formula -> `target.modeFlags.isBoss` always undefined
- **Issue**: The `modeFlags.isBoss` is never set (VD-6), so the damage formula always classifies targets as `'normal'`. This breaks:
  - Offensive: Abysmal Knight (+25% vs boss), Turtle General (+20% vs all)
  - Defensive: Alice Card (-40% from boss), Tirfing Card (-5% from boss)
  - Magic: Vesper Card (ignore boss MDEF)
- **Note**: Other boss checks throughout `index.js` correctly use `enemy.monsterClass === 'boss' || enemy.monsterClass === 'mvp'` or `modeFlags.statusImmune` -- those work fine. ONLY the damage formula's card modifier system is affected.

### CS-4: Monster Soft Defense Inconsistency Across 4 Code Paths
- **Paths**:
  1. Main spawn (line 4465): Player formula `VIT*0.5 + max(0, VIT-20)*0.3`
  2. Metamorphosis respawn (line 29688): Same player formula
  3. Abracadabra summon (line 16024): Raw `VIT` (no formula at all)
  4. Abracadabra metamorphosis (line 16061): Raw `VIT`
- **Correct**: `floor(VIT/2)` for softDef, `floor((INT + Level) / 4)` for softMDef
- **All 4 paths are wrong**, and they're wrong in DIFFERENT ways.

---

## Contradictions Between Research Docs

### CONTRA-1: ASPD Attack Delay Multiplier
- **AUDIT_01** (C5-2): Claims research says `(200-ASPD) * 10` ms
- **AUDIT_02** (P6): Claims research says `(200-ASPD) * 20` ms
- **Deep Research `06_ASPD_Hit_Flee_Critical.md`**: Lines 249 and 287 say `* 20`. Line 255 says `* 10` for "Amotion" (internal representation). Line 259 explains: the `* 20` vs `* 10` discrepancy is unit conversion -- `* 20` gives correct milliseconds.
- **Resolution**: `* 20` is correct for milliseconds. AUDIT_01 was wrong to cite `* 10`.

### CONTRA-2: Soft DEF Formula
- **AUDIT_01** (C3): Says server formula `VIT/2 + AGI/5 + BaseLv/2` "matches" the display formula (calls it IMPLEMENTED with random component MISSING)
- **AUDIT_02** (P3): Says the same formula is "COMPLETELY WRONG" and should be `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)` (omitting AGI and BaseLv)
- **Deep Research `01_Stats_Attributes.md`** (line 522): Full formula is `vit_def_base + vit_def_random + floor(AGI / 5) + floor(BaseLevel / 2)` -- includes AGI and BaseLv.
- **Resolution**: AUDIT_01 is correct. AUDIT_02 P3 cites an incomplete formula that omits AGI/BaseLv components. The server's base formula matches the deterministic display formula exactly. The only missing piece is the per-hit random VIT component.

### CONTRA-3: Hit Rate Cap
- **AUDIT_01** (C4-3): Says server caps at 95% while research says 100% max hit rate
- **AUDIT_02** (Step 3): Says clamped to 5-95% is "CORRECT"
- **Deep Research**: Multiple sources say 5-95% clamp (with 5% minimum miss chance)
- **Resolution**: 95% cap is CORRECT. AUDIT_01 was wrong about 100% being the cap.

### CONTRA-4: Flee Penalty Threshold
- **AUDIT_01** (C4-4): Says penalty should start at 2 attackers (`numAttackers - 1`)
- **AUDIT_03** (within AUDIT_03 scope): Does not address this
- **Code**: `numAttackers > 2` means penalty starts at 3 attackers
- **Deep Research `06_ASPD_Hit_Flee_Critical.md`**: Needs verification, but the common rAthena implementation starts at 2 attackers
- **Resolution**: The server starts penalty at 3+ attackers. RO Classic starts at 2+. This is a REAL discrepancy, severity LOW.

---

## Final Prioritized Fix List

### Tier 1: Critical (Affects all combat correctness)

| # | Issue | Location | Impact | Fix Complexity |
|---|-------|----------|--------|----------------|
| 1 | **Crit bypass DEF** (VD-3) | `ro_damage_formulas.js:749` | LUK/crit builds dramatically underperform | 1 line: add `if (isCritical) skipDEF = true;` |
| 2 | **StatusATK missing BaseLv/4** (VD-1) | `ro_damage_formulas.js:288-290` | -24 ATK at level 99 on ALL physical attacks | 2 lines: add `Math.floor(level / 4) +` to both formulas |
| 3 | **StatusATK LUK/5 -> LUK/3** (VD-2) | `ro_damage_formulas.js:289-290` | -14 ATK at 99 LUK | 2 lines: change `/5` to `/3` |
| 4 | **Monster soft MDEF formula** (VD-4) | `index.js:4466, 29689, 16024, 16062` | Magic deals far less damage to monsters than intended | 4 locations: change to `floor((INT + Level) / 4)` |
| 5 | **modeFlags.isBoss never set** (VD-6) | `index.js:4427-4431` | Boss card bonuses (Abysmal Knight, Alice, etc.) broken | 1 line: add `modeFlags.isBoss = true;` in boss protocol |

### Tier 2: High (Affects specific combat subsystems)

| # | Issue | Location | Impact | Fix Complexity |
|---|-------|----------|--------|----------------|
| 6 | **Monster soft DEF formula** (VD-5) | `index.js:4465, 29688, 16024, 16061` | Physical damage slightly reduced vs high-VIT monsters | 4 locations: change to `floor(VIT / 2)` |
| 7 | **Magic missing defensive cards** (VD-10) | `ro_damage_formulas.js:919-1054` | Shield cards don't reduce magic damage | ~20 lines: add cardDefMods block to magic function |
| 8 | **Freeze/stone element override in physical** (VD-11, ND-1) | `index.js:3905` | Physical attacks use wrong element vs frozen/stoned targets | ~5 lines: apply overrideElement in getEnemyTargetInfo |
| 9 | **Negative element = heal** (VD-7) | `ro_damage_formulas.js:720-726, 982-986` + combat tick | Poison vs Undead should heal, currently does 0 | ~15 lines: add healing path in combat tick |

### Tier 3: Medium (Balance/correctness improvements)

| # | Issue | Location | Impact | Fix Complexity |
|---|-------|----------|--------|----------------|
| 10 | **Provoke boosts magic** (VD-9) | `ro_damage_formulas.js:960-961` | Provoke incorrectly +32% to magic damage | 2 lines: skip atkMultiplier in magic path |
| 11 | **PD gate on !isSkill** (FP-4) | `ro_damage_formulas.js:485` | Skills without forceHit can be Perfect Dodged | 1 line: add `&& !isSkill` to PD condition |
| 12 | **PD/Crit check order** | `ro_damage_formulas.js:480-520` | PD can negate natural crits | ~20 lines: swap crit and PD check blocks |
| 13 | **Abracadabra summon soft DEF** (ND-2) | `index.js:16024, 16061-16062` | Summoned monsters have wrong defenses | 4 lines: use correct formulas |

### Tier 4: Low (Polish, design decisions, future-proofing)

| # | Issue | Location | Impact | Fix Complexity |
|---|-------|----------|--------|----------------|
| 14 | **SoftDEF random VIT component** | `ro_damage_formulas.js:319` | Damage more consistent than RO Classic | ~10 lines: add random component |
| 15 | **ASPD *50 vs *20** (VD-8) | `index.js:460` | Intentional design -- document it | 0 code, 1 comment |
| 16 | **Flee penalty at 3+ vs 2+** | `ro_damage_formulas.js:397` | Low impact on gameplay | 1 line: change `> 2` to `> 1` |
| 17 | **MD_IGNORE* flags** | `index.js:355-432` | No current monsters use these | ~30 lines: add to parse + damage |
| 18 | **Defensive card order** | `ro_damage_formulas.js:818-834` | Off by 1 in rare cases | ~10 lines: reorder blocks |
| 19 | **Hard DEF formula (4000-based)** | `ro_damage_formulas.js:776` | Design decision -- would require recalibrating all monster DEF | Large if switching |

### Summary Statistics

| Category | Count |
|----------|-------|
| **Verified Real (from audits)** | 11 |
| **False Positives (audit wrong)** | 6 |
| **Newly Discovered** | 5 |
| **Cross-System Issues** | 4 |
| **Research Doc Contradictions** | 4 |
| **Tier 1 Critical Fixes** | 5 |
| **Tier 2 High Fixes** | 4 |
| **Tier 3 Medium Fixes** | 4 |
| **Tier 4 Low/Design Fixes** | 6 |

### Effort Estimate for Tier 1+2 Fixes

The 9 highest-priority fixes (Tier 1 + Tier 2) collectively require changes to:
- `ro_damage_formulas.js`: ~30 lines changed
- `index.js`: ~15 lines changed across 8 locations

Total estimated effort: **1-2 hours** for a developer familiar with the codebase. All fixes are surgical (specific line changes) with no architectural refactoring needed.
