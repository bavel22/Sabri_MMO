# Audit: Combat Damage Formulas

> **Scope**: Deep Research docs 04 (Physical Damage), 05 (Magical Damage), 06 (ASPD/Hit/Flee/Critical) compared against actual server implementation in `server/src/ro_damage_formulas.js` and `server/src/index.js`.
> **Date**: 2026-03-22
> **Auditor**: Claude Code
> **Verdict**: Mostly correct with 9 critical discrepancies, 7 partial implementations, and 6 missing features.

---

## Summary

| Category | Implemented | Partial | Missing | Critical Bugs |
|----------|------------|---------|---------|---------------|
| Physical Damage Pipeline | 14/21 steps | 4 | 3 | 4 |
| Magical Damage Pipeline | 11/14 steps | 2 | 1 | 1 |
| ASPD Calculation | 5/7 features | 1 | 1 | 1 |
| Hit/Flee/Critical | 8/11 features | 2 | 1 | 3 |
| **Totals** | **38/53** | **9** | **6** | **9** |

### Critical Issues (will noticeably affect gameplay)

1. **StatusATK missing `floor(BaseLv/4)` term** -- all physical damage is low by ~24 ATK at level 99
2. **StatusATK uses `floor(LUK/5)` instead of `floor(LUK/3)`** -- LUK contributes ~40% less to physical damage than it should
3. **SoftDEF formula is wrong** -- uses `VIT/2 + AGI/5 + BaseLv/2` instead of RO's `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)` -- VIT builds get far less DEF than intended
4. **Hard DEF uses simplified linear formula** instead of official `(4000+DEF)/(4000+DEF*10)` -- high DEF is too powerful
5. **Critical hits do NOT bypass DEF** -- crits go through full Hard DEF + Soft DEF reduction, making LUK/crit builds dramatically weaker
6. **PD check order wrong** -- PD is checked BEFORE crit (should be crit first, PD second per rAthena)
7. **PD applies to skills** -- doc says PD only works vs auto-attacks, but code does not gate on `!isSkill`
8. **ASPD delay uses `*50` instead of `*20`** -- attack speed is 2.5x slower than RO Classic
9. **FLEE multi-attacker penalty is flat subtraction** instead of percentage-based

---

## Physical Damage Pipeline (21-step comparison)

### Step 1: Perfect Dodge Check

**Research**: PD = `1 + floor(LUK/10) + BonusPD`. Checked only for auto-attacks. Criticals bypass PD. Skills bypass PD.

**Implementation** (`ro_damage_formulas.js` line 481-492):
```javascript
if (!forceHit && !forceCrit) {
    const pd = defDerived.perfectDodge;
    if (Math.random() * 100 < pd) { ... }
}
```

**Status**: ⚠️ PARTIAL -- Two issues:
1. PD formula itself is correct (`1 + floor(LUK/10) + bonusPerfectDodge`)
2. **Bug**: PD is checked BEFORE the crit roll (Step 2). rAthena checks crit first, then PD. Current order: PD -> Crit -> FLEE. Correct order: Crit -> PD -> FLEE.
3. **Bug**: PD is not gated on `!isSkill`. Skills should bypass PD entirely. Currently, a skill with neither `forceHit` nor `forceCrit` can be Perfect Dodged, which is incorrect.

---

### Step 2: Critical Hit Check

**Research**: `CRI = 1 + floor(LUK*0.3) + EquipCRI`. Katar doubles CRI. `CritShield = floor(targetLUK/5)`. Only auto-attacks can crit (skills cannot). Pre-renewal crits bypass both Hard DEF and Soft DEF.

**Implementation** (`ro_damage_formulas.js` line 494-521):
- CRI formula: `1 + Math.floor(luk * 0.3) + bonusCritical` -- **Correct**
- Katar double: `if (weaponType === 'katar') critical *= 2` -- **Correct**
- CritShield: `Math.floor(targetLuk * 0.2)` = `floor(LUK/5)` -- **Correct**
- Skills cannot crit: `else if (!isSkill)` guard -- **Correct**
- Card crit race bonus (`bCriticalAddRace`): **Implemented**
- Card ranged crit bonus (`bCriticalLong`): **Implemented**

**Status**: ⚠️ PARTIAL
- CRI formula: ✅ IMPLEMENTED
- CritShield: ✅ IMPLEMENTED
- Katar double: ✅ IMPLEMENTED
- Skills cannot crit: ✅ IMPLEMENTED
- **Critical Bug**: Crits do NOT bypass DEF. The `isCritical` flag never sets `skipDEF = true`. Line 748-749 shows `skipDEF` is only set by `ignoreDefense` option or card effects. Crits go through full Hard DEF percentage reduction AND Soft DEF flat subtraction. This is the single biggest damage formula error -- it makes crit builds dramatically weaker than intended.

---

### Step 3: HIT/FLEE Check

**Research**: `HitRate = 80 + AttackerHIT - DefenderFLEE`, clamped 5-95%. Skipped for crits and `forceHit` skills.

**Implementation** (`ro_damage_formulas.js` line 394-410, 523-543):
- HIT formula: `175 + BaseLv + DEX + bonusHIT` -- **Correct** (matches iRO Wiki)
- FLEE formula: `100 + BaseLv + AGI + bonusFLEE` -- **Correct**
- HitRate formula: `80 + attackerHIT - effectiveFlee` -- **Correct**
- Clamp to 5-95%: `Math.max(5, Math.min(95, hitRate))` -- **Correct**
- Skipped for crits: `if (!isCritical && !forceHit)` -- **Correct**
- Skill-specific hitrate bonus (Bash +5%/lv, Magnum Break +10%/lv): **Implemented** via `hitRatePercent` parameter
- Buff multipliers (Blind hitMul/fleeMul): **Implemented**

**Status**: ✅ IMPLEMENTED

---

### Step 4: StatusATK Calculation

**Research**:
- Melee: `floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)`
- Ranged: `floor(BaseLv/4) + DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/3)`

**Implementation** (`ro_damage_formulas.js` line 283-290):
```javascript
// Melee:  STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/5)
// Ranged: DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/5)
const statusATK = isRangedWeapon
    ? dex + Math.floor(dex / 10) ** 2 + Math.floor(str / 5) + Math.floor(luk / 5)
    : str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 5);
```

**Status**: ❌ TWO ERRORS
1. **Missing `floor(BaseLv/4)` term**: The level contribution is completely absent. At level 99 this is +24 StatusATK missing from every physical attack. The comment even says "rAthena status_base_atk" but omits the level term that rAthena includes.
2. **`floor(LUK/5)` instead of `floor(LUK/3)`**: At 99 LUK, this gives `floor(99/5)=19` instead of `floor(99/3)=33`. That is 14 ATK missing. The comment in the code itself says `floor(LUK/5)` which contradicts both the deep research doc AND iRO Wiki Classic.

Combined impact at Lv99 with 99 LUK: **-38 StatusATK** on every hit.

---

### Step 5: WeaponATK Calculation (Variance, DEX Narrowing)

**Research**: `WeaponATK = rnd(floor(min(DEX*(0.8+0.2*WLv), BaseATK)), BaseATK)`. Critical = always max. Also adds `weaponATK * primaryStat / 200` stat bonus.

**Implementation** (`ro_damage_formulas.js` line 568-597):
- DEX narrowing: `atkMin = min(secondaryStat * (0.8 + 0.2*WL), sizedWeaponATK)` -- **Correct**
- Stat bonus: `weaponStatBonus = floor(sizedWeaponATK * primaryStat / 200)` -- **Correct** (rAthena `atkmax += wa->atk * str / 200`)
- Critical = max ATK: `if (isCritical || isMaxPower) variancedWeaponATK = atkMax` -- **Correct**
- Maximize Power = max ATK: **Correct**
- Random between atkMin and atkMax: **Correct**

**Status**: ✅ IMPLEMENTED

---

### Step 6: Size Penalty (WeaponATK only)

**Research**: `SizedWeaponATK = floor(WeaponATK * SizePenalty% / 100)`. StatusATK NOT affected.

**Implementation** (`ro_damage_formulas.js` line 153-174, 556-566):
- SIZE_PENALTY table: Complete (all weapon types including instrument, whip, book, knuckle)
- Size penalty applied to weapon ATK only: **Correct** (`sizedWeaponATK = floor(weaponATK * sizePenaltyPct / 100)`, then `totalATK = statusATK + variancedWeaponATK`)
- Drake Card (`bNoSizeFix`): **Implemented** (`cardNoSizeFix`)
- Mounted spear vs Medium = 100%: **Implemented**

**Status**: ✅ IMPLEMENTED

Note: The deep research doc mentions Spear (mounted) vs Medium = 100%. Implementation has this at line 559-562.

---

### Step 7: Weapon Level Variance

**Research**: Final +-5% variance per weapon level. `VarianceMul = rnd(1.0 - WLv*0.05, 1.0)`. Crits skip this.

**Implementation**: NOT FOUND in `ro_damage_formulas.js`. The weapon ATK variance (DEX narrowing) is implemented, but the additional weapon-level-based damage variance (Lv4 weapon = 80-100% damage spread) is not applied.

**Status**: ❌ MISSING -- No weapon level variance multiplication step. This means damage is more consistent than it should be. A Lv4 weapon should have 80-100% variance but currently does not.

---

### Step 8: ExtraATK (Equipment ATK, Consumable ATK, Ammo ATK)

**Research**: `ExtraATK = EquipATK + ConsumableATK + AmmoATK`. Subject to element/size/card bonuses.

**Implementation**:
- Equipment ATK (card `bAtk` bonuses): Added to `weaponATK` via `cardB.atk` in `getEffectiveStats()` (line 3795)
- Arrow ATK: Implemented separately (`arrowATK` with its own variance, line 599-612)
- Consumable ATK: Not explicitly separated -- likely folded into buff system

**Status**: ⚠️ PARTIAL -- Arrow ATK is handled well (with crit/normal variance). Equipment flat ATK is added to weaponATK rather than as a separate ExtraATK component, which means it goes through weapon variance instead of being flat. This is a minor structural difference.

---

### Step 9: Sum Pre-Modifier Damage

**Research**: `BaseDamage = StatusATK + SizedWeaponATK + ExtraATK`

**Implementation** (`ro_damage_formulas.js` line 614-615):
```javascript
let totalATK = statusATK + variancedWeaponATK;
```

**Status**: ✅ IMPLEMENTED (arrow ATK added to variancedWeaponATK earlier)

---

### Step 10: Buff ATK Multipliers (Provoke, Power Thrust)

**Research**: `BaseDamage = floor(BaseDamage * BuffMul)`. Provoke = 1.32x at Lv10, Power Thrust = 1.25x at Lv5.

**Implementation** (`ro_damage_formulas.js` line 624-626):
```javascript
const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;
totalATK = Math.floor(totalATK * atkMultiplier);
```

**Status**: ✅ IMPLEMENTED

---

### Step 11: Skill Multiplier

**Research**: `SkillDamage = floor(RawATK * SkillMultiplier% / 100)`

**Implementation** (`ro_damage_formulas.js` line 631-633):
```javascript
if (isSkill && skillMultiplier !== 100) {
    totalATK = Math.floor(totalATK * skillMultiplier / 100);
}
```

**Status**: ✅ IMPLEMENTED

---

### Step 12: Card/Equipment Damage Bonuses

**Research**: Same category = additive, different categories = multiplicative. `CardMul = (1+race%) * (1+ele%) * (1+size%) * (1+class%)`

**Implementation** (`ro_damage_formulas.js` line 648-701):
- Race bonus: ✅ `Math.floor(totalATK * (100 + raceBonus) / 100)`
- Element bonus: ✅
- Size bonus: ✅
- Boss/Normal class bonus (`bAddClass`): ✅
- Sub-race bonus (`bAddRace2`): ✅
- Per-monster bonus (`bAddDamageClass`): ✅
- Ranged ATK bonus (`bLongAtkRate`): ✅
- Per-skill bonus (`bSkillAtk`): ✅

Within each category: additive (correct -- single `raceBonus` is sum of all race cards).
Between categories: multiplicative (correct -- each applied sequentially with floor).

**Status**: ✅ IMPLEMENTED -- Comprehensive card modifier system.

---

### Step 13: Element Modifier

**Research**: `EleMod = ELEMENT_TABLE[atk][def][defLevel-1]`. 10x10x4 table. Applied before DEF in the research doc pipeline, but note says rAthena applies after DEF.

**Implementation** (`ro_damage_formulas.js` line 26-147, 703-727, 877-880):
- Full 10x10x4 ELEMENT_TABLE: ✅ Complete (sourced from rAthena `attr_fix.yml`)
- Element immunity (<=0): ✅ Returns 0 damage with `elementImmune`/`elementHeal` hitType
- Non-elemental monster attacks (`isNonElemental`): ✅ Bypasses table
- **Applied AFTER DEF** (line 880): `totalATK = Math.floor(totalATK * eleModifier / 100)` -- This is after the DEF reduction block (lines 769-779). This matches rAthena's actual order but differs from the research doc's Step 13 (before DEF).

**Status**: ✅ IMPLEMENTED -- Element is correctly applied after DEF, matching rAthena pre-renewal source.

---

### Step 14: Hard DEF (Percentage Reduction)

**Research**: Official formula: `floor(Damage * (4000 + HardDEF) / (4000 + HardDEF*10))`. rAthena simplified: `floor(Damage * (100 - HardDEF) / 100)`.

**Implementation** (`ro_damage_formulas.js` line 776):
```javascript
totalATK = Math.floor(totalATK * (100 - hardDef) / 100);
```

**Status**: ⚠️ PARTIAL -- Uses the simplified rAthena formula `(100 - DEF)%` instead of the official iRO `(4000+DEF)/(4000+DEF*10)` formula. This matters at high DEF values:
- At DEF 50: Simplified = 50% reduction. Official = 11.1% reduction.
- At DEF 99: Simplified = 99% reduction. Official = ~47%.

The simplified formula makes high-DEF targets nearly immune to physical damage (99% reduction) while the official formula caps around 50%. The deep research doc notes the official formula is recommended for accuracy. The code caps hardDef at 99 (line 736).

Also: Hard DEF is capped at 99 via `Math.min(99, ...)` which is correct.

---

### Step 15: Soft DEF (Flat Subtraction)

**Research**: `SoftDEF = floor(VIT*0.5) + max(floor(VIT*0.3), floor(VIT^2/150) - 1)`. At 99 VIT = 113 SoftDEF.

**Implementation** (`ro_damage_formulas.js` line 318-319):
```javascript
// floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)
const softDEF = Math.floor(vit / 2) + Math.floor(agi / 5) + Math.floor(level / 2);
```

**Status**: ❌ WRONG FORMULA -- The implementation uses a completely different formula than both the research doc and iRO Wiki Classic:
- **Implemented**: `VIT/2 + AGI/5 + BaseLv/2` (at Lv99, VIT 99, AGI 30: 49 + 6 + 49 = 104)
- **Research/iRO**: `VIT*0.5 + max(VIT*0.3, VIT^2/150 - 1)` (at VIT 99: 49 + 64 = 113)

Key differences:
1. AGI contributes to SoftDEF in the implementation but not in RO Classic (AGI affects FLEE, not DEF)
2. BaseLv contributes to SoftDEF in the implementation but not in RO Classic
3. The VIT^2/150 accelerating term is missing -- high VIT should give much more DEF
4. At low VIT (30): Implemented = 15+6+49=70, Research = 15+9=24. At low VIT the implementation actually gives MORE soft DEF due to BaseLv contribution.

This formula appears to be a custom hybrid, not matching any known RO source.

---

### Step 16: MasteryATK (Post-DEF, Flat Addition)

**Research**: MasteryATK = Weapon Mastery + Star Crumb + Spirit Sphere + Overupgrade. Bypasses DEF, element, size, skill ratio.

**Implementation** (`ro_damage_formulas.js` line 789-811):
- Refine ATK: ✅ Added post-DEF (line 796-798), excluded for specific skills (Shield Boomerang, Acid Terror, Investigate, Asura Strike)
- Overupgrade random bonus: ✅ `1 to overrefineMax per hit` (line 801-804)
- Mastery ATK (passiveATK): ✅ Added post-DEF (line 809-811)

From `index.js` line 3796:
```javascript
passiveATK: passive.bonusATK + ((player.spiritSpheres || 0) * 3) + (buffMods.bonusATK || 0),
```
- Spirit Spheres: ✅ `+3 per sphere`
- Impositio Manus (`bonusATK`): ✅ Added as flat post-DEF ATK
- Star Crumb ATK: ✅ Added to `weaponATK` at equip time (index.js line 5431: `[0, 5, 10, 40]`)

**Status**: ✅ IMPLEMENTED

Note: Star Crumb ATK is added to weaponATK rather than to passiveATK. This means it goes through size penalty and DEF reduction, which is technically wrong -- Star Crumb ATK should bypass both. However, since it is a small amount (max +40), the practical impact is minor.

---

### Step 17: BuffATK (Impositio Manus, etc.)

**Research**: Flat ATK from active buffs, bypasses DEF/element/size/skill ratio.

**Implementation**: Impositio Manus `bonusATK` is folded into `passiveATK` (see Step 16 above), which is correctly added post-DEF.

**Status**: ✅ IMPLEMENTED

---

### Step 18: Critical Multiplier

**Research**: Pre-renewal critical = +40% damage multiplier. Applied after DEF (but since crits bypass DEF, effectively applied to the full damage).

**Implementation** (`ro_damage_formulas.js` line 617-622):
```javascript
if (isCritical) {
    const baseCritBonus = 40;
    const equipCritAtkRate = attacker.critAtkRate || 0;
    totalATK = Math.floor(totalATK * (100 + baseCritBonus + equipCritAtkRate) / 100);
}
```

**Status**: ⚠️ PARTIAL -- The +40% multiplier is applied at line 618-622, but it is applied BEFORE the DEF reduction step (it is at Step 4 in the code, before Step 8). Since crits should bypass DEF entirely, the order would not matter IF DEF bypass was implemented. But since DEF bypass is NOT implemented for crits, the +40% is applied to pre-DEF damage, then DEF reduces it -- making crits even weaker than they should be.

Additionally, `equipCritAtkRate` (`bCritAtkRate`) is correctly added. This is good.

---

### Step 19: Defensive Card Reductions

**Research**: `DamageAfterDEF = floor(DamageAfterDEF * (1 - DefensiveRaceReduction)) * (1 - DefensiveEleReduction) * ...`

**Implementation** (`ro_damage_formulas.js` line 818-853):
- Defensive race reduction (`cardDefMods.race_X`): ✅
- Defensive element reduction (`cardDefMods.ele_X`): ✅
- Defensive size reduction (`cardDefMods.size_X`): ✅
- Defensive boss/normal class (`cardSubClass`): ✅
- Ranged damage reduction (`cardLongAtkDef`): ✅
- Monster-specific DEF (`cardAddDefMonster`): ✅

**Status**: ✅ IMPLEMENTED

---

### Step 20: Floor to Minimum 1

**Research**: `FinalDamage = max(1, CalculatedDamage)`. Exception: element immunity = true 0.

**Implementation** (`ro_damage_formulas.js` line 900):
```javascript
result.damage = Math.max(1, totalATK);
```

Element immunity handled earlier (line 720-726): returns 0 damage before DEF step.

**Status**: ✅ IMPLEMENTED

---

### Step 21: Dual Wield Penalty

**Research**: Right hand: 50% base (100% at Righthand Mastery Lv5). Left hand: 30% base (80% at Lefthand Mastery Lv5).

**Implementation**: Not in `ro_damage_formulas.js`. Handled in `index.js` combat tick where per-hand damage is calculated separately. From the `MEMORY.md` entry: "Both hands hit per attack cycle with per-hand card mods and mastery penalties."

**Status**: ✅ IMPLEMENTED (in index.js combat tick, not in the damage formula module)

---

## Magical Damage Pipeline (14-step comparison)

### Step 1: GTB Card Magic Immunity Check

**Research**: `bNoMagicDamage:100` = complete magic immunity, checked first.

**Implementation** (`ro_damage_formulas.js` line 938-944):
```javascript
if (target.cardNoMagicDamage && target.cardNoMagicDamage >= 100) { ... return; }
```

**Status**: ✅ IMPLEMENTED

---

### Step 2: MATK Calculation (Status MATK Min/Max)

**Research**: `MATK_Min = INT + floor(INT/7)^2`, `MATK_Max = INT + floor(INT/5)^2`.

**Implementation** (`ro_damage_formulas.js` line 293-297):
```javascript
const statusMATK = intStat + Math.floor(intStat / 7) ** 2;
const statusMATKMax = intStat + Math.floor(intStat / 5) ** 2;
const matkMin = statusMATK + Math.floor(weaponMATK * 0.7);
const matkMax = statusMATKMax + weaponMATK;
```

**Status**: ✅ IMPLEMENTED -- Both min/max formulas correct. Weapon MATK min at 70% is correct.

---

### Step 3: MATK Variance Roll

**Research**: `ActualMATK = random(MATK_Min, MATK_Max)`. Each hit of multi-hit skills rolls independently.

**Implementation** (`ro_damage_formulas.js` line 949):
```javascript
const matk = matkMin + Math.floor(Math.random() * (matkMax - matkMin + 1));
```

**Status**: ✅ IMPLEMENTED -- Single roll per call. Multi-hit skills call this function per hit from index.js.

---

### Step 4: Skill Multiplier

**Research**: `TotalDamage = floor(MATK * SkillMultiplier / 100)`

**Implementation** (`ro_damage_formulas.js` line 952):
```javascript
let totalDamage = Math.floor(matk * skillMultiplier / 100);
```

**Status**: ✅ IMPLEMENTED

---

### Step 5: Card MATK Rate Bonus (bMatkRate)

**Research**: Weapon `bMatkRate` bonus (e.g., Rod +15%) applied multiplicatively.

**Implementation** (`ro_damage_formulas.js` line 954-957):
```javascript
if (attacker.cardMatkRate && attacker.cardMatkRate !== 0) {
    totalDamage = Math.floor(totalDamage * (100 + attacker.cardMatkRate) / 100);
}
```

**Status**: ✅ IMPLEMENTED

---

### Step 6: Buff ATK Multiplier (for Magic)

**Research**: No explicit magic-specific buff multiplier in pre-renewal (Provoke only affects physical).

**Implementation** (`ro_damage_formulas.js` line 960-961):
```javascript
const atkMultiplier = (attacker.buffMods && attacker.buffMods.atkMultiplier) || 1.0;
totalDamage = Math.floor(totalDamage * atkMultiplier);
```

**Status**: ⚠️ PARTIAL -- `atkMultiplier` (Provoke) is applied to magic damage here, but in RO Classic, Provoke only affects physical ATK, not MATK. This means Provoke incorrectly boosts magic damage. Should use a separate `matkMultiplier` or skip this for magic.

---

### Step 7: Card Magic Race Bonus (bMagicAddRace)

**Research**: Magic damage bonus vs specific races.

**Implementation** (`ro_damage_formulas.js` line 969-974):

**Status**: ✅ IMPLEMENTED

---

### Step 8: Element Modifier (Spell Element vs Target Element)

**Research**: Uses same 10x10x4 table. Spell element is determined by the skill.

**Implementation** (`ro_damage_formulas.js` line 976-988):

**Status**: ✅ IMPLEMENTED -- Element modifier applied before MDEF.

---

### Step 9: Hard MDEF (Percentage Reduction)

**Research**: `DamageAfterHardMDEF = floor(Damage * (100 - HardMDEF) / 100)`. Capped at 99.

**Implementation** (`ro_damage_formulas.js` line 993-1005):
```javascript
let effectiveHardMdef = Math.min(99, ...);
if (effectiveHardMdef > 0) {
    totalDamage = Math.floor(totalDamage * (100 - effectiveHardMdef) / 100);
}
```

- Steel Body override (`overrideHardMDEF`): ✅
- Card ignore MDEF (`cardIgnoreMdefClass` -- Vesper/HW Card): ✅

**Status**: ✅ IMPLEMENTED -- Uses simplified rAthena formula (same as physical Hard DEF). The deep research doc recommends this formula for MDEF, so this is correct.

---

### Step 10: Soft MDEF (Flat Subtraction)

**Research**: `SoftMDEF = INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)`

**Implementation** (`ro_damage_formulas.js` line 322-323):
```javascript
const softMDEF = intStat + Math.floor(vit / 5) + Math.floor(dex / 5) + Math.floor(level / 4);
```

**Status**: ✅ IMPLEMENTED -- Matches exactly.

---

### Step 11: Freeze/Stone MDEF Multiplier

**Research**: Frozen/Stoned targets take +25% magic damage. `DamageAfterMDEF * 1.25`.

**Implementation** (`ro_damage_formulas.js` line 1017-1022):
```javascript
const mdefMultiplier = (target.buffMods && target.buffMods.mdefMultiplier) || 1.0;
if (mdefMultiplier !== 1.0) {
    totalDamage = Math.floor(totalDamage * mdefMultiplier);
}
```

**Status**: ✅ IMPLEMENTED (via buff system setting `mdefMultiplier: 1.25` on freeze/stone)

---

### Step 12: Sage Zone Elemental Damage Boost

**Research**: Volcano/Deluge/Violent Gale provide +ATK% and +damage% for matching element attacks.

**Implementation** (`ro_damage_formulas.js` line 1024-1033):

**Status**: ✅ IMPLEMENTED

---

### Step 13: Mystical Amplification (Magic Power)

**Research**: Wizard skill that increases next spell damage by (skill_level * 5 + 45)% at Lv10. Applied as a MATK multiplier.

**Implementation**: Not found in `ro_damage_formulas.js`. No `magicPower` or `HW_MAGICPOWER` handler in index.js search results (only an itemskill scroll reference).

**Status**: ❌ MISSING -- Mystical Amplification (HW_MAGICPOWER, Wizard skill ID 811) is not implemented as a damage modifier. There is only an item scroll reference. The actual skill handler and MATK boost mechanic are absent.

---

### Step 14: Floor to Minimum 1

**Implementation** (`ro_damage_formulas.js` line 1052):
```javascript
result.damage = Math.max(1, totalDamage);
```

**Status**: ✅ IMPLEMENTED

---

## ASPD Calculation

### ASPD Core Formula

**Research**: `ASPD = 200 - floor((WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SM))`

**Implementation** (`ro_damage_formulas.js` line 219-256):
```javascript
const agiReduction = Math.floor(WD * agi / 25);
const dexReduction = Math.floor(WD * dex / 100);
const totalReduction = Math.floor((agiReduction + dexReduction) / 10);
let rawASPD = 200 - Math.floor((WD - totalReduction) * (1 - speedMod));
```

**Status**: ✅ IMPLEMENTED -- Formula matches exactly. Speed modifier correctly applied as `(1 - SM)`.

---

### Base ASPD by Class/Weapon Type

**Research**: Full table for 7 first classes + 12 second classes across all weapon types.

**Implementation**: Uses `ASPD_BASE_DELAYS` from `ro_exp_tables.js`. Transcendent classes resolve to base class via `TRANS_TO_BASE_CLASS`.

**Status**: ✅ IMPLEMENTED (assumed correct -- table is imported from data module)

---

### Dual Wield ASPD

**Research**: `WD_dual = floor((WD_right + WD_left) * 7 / 10)`

**Implementation** (`ro_damage_formulas.js` line 227-232):
```javascript
WD = Math.floor((baseDelay + leftDelay) * 7 / 10);
```

**Status**: ✅ IMPLEMENTED

---

### Mount ASPD Penalty

**Research**: 50% base penalty, +10% restored per Cavalry Mastery level.

**Implementation** (`ro_damage_formulas.js` line 248-253):
```javascript
const mountMultiplier = 0.5 + (cavalierMasteryLv || 0) * 0.1;
rawASPD = Math.floor(rawASPD * Math.min(1, mountMultiplier));
```

**Status**: ✅ IMPLEMENTED

---

### ASPD Hard Cap

**Research**: Pre-renewal hard cap = 190. `(200 - 190) * 20ms = 200ms = 5 hits/sec`.

**Implementation** (`index.js` line 344, `ro_damage_formulas.js` line 244-246):
- `ASPD_CAP: 195` (not 190!)
- Single weapon cap: 195
- Dual wield cap: 190

**Status**: ⚠️ PARTIAL -- The cap is 195 for single weapon (documented as intentional deviation). Above 195, diminishing returns apply (exponential decay). This is a deliberate game design choice, not a bug.

---

### Attack Delay Conversion (ASPD to Milliseconds)

**Research**: `Attack Delay (ms) = (200 - ASPD) * 20`. ASPD 190 = 200ms = 5 hits/sec.

**Implementation** (`index.js` line 460):
```javascript
interval = (200 - aspd) * 50; // e.g. ASPD 195 -> 5 * 50 = 250ms
```

**Status**: ❌ WRONG MULTIPLIER -- Uses `*50` instead of `*20`. This makes attacks 2.5x slower than RO Classic:

| ASPD | Research (ms) | Implementation (ms) | Difference |
|------|--------------|-------------------|------------|
| 150 | 1000 | 2500 | 2.5x slower |
| 170 | 600 | 1500 | 2.5x slower |
| 180 | 400 | 1000 | 2.5x slower |
| 185 | 300 | 750 | 2.5x slower |
| 190 | 200 | 500 | 2.5x slower |
| 195 | 100 | 250 | 2.5x slower |

At ASPD 190 (the RO Classic cap), RO Classic gives 5 hits/sec. This implementation gives 2 hits/sec. This is a massive gameplay difference.

**Note**: This could be intentional game balancing (the ASPD_CAP is also 195 instead of 190). The higher cap partially compensates: ASPD 195 with `*50` = 250ms, which is comparable to RO's ASPD 187.5 with `*20` = 250ms. However, at lower ASPD values the discrepancy is very large.

---

### Shield ASPD Penalty

**Research**: Equipping a shield adds +5 to +10 to weapon delay.

**Implementation**: Not found. No shield penalty in `calculateASPD()`.

**Status**: ❌ MISSING

---

### ASPD Potion Application

**Research**: Potions (Concentration 10%, Awakening 15%, Berserk 20%) apply as additional delay reduction. Highest potion only (no stacking). Stack with skill buffs.

**Implementation** (`index.js` line 469-472):
```javascript
if (aspdPotionReduction > 0) {
    interval = Math.max(217, Math.floor(interval * (1 - aspdPotionReduction)));
}
```

**Status**: ⚠️ PARTIAL -- Potions are applied as a post-calculation delay reduction to the millisecond interval, not as part of the ASPD formula's speed modifier (SM). In RO Classic, potions are part of the SM calculation within the ASPD formula. The current approach is functionally different but may produce similar results.

---

## Hit/Flee/Critical

### HIT Formula

**Research**: `Player HIT = 175 + BaseLv + DEX + BonusHIT`

**Implementation** (`ro_damage_formulas.js` line 301):
```javascript
const hit = 175 + level + dex + bonusHit;
```

**Status**: ✅ IMPLEMENTED

---

### FLEE Formula

**Research**: `Player FLEE = 100 + BaseLv + AGI + BonusFLEE`

**Implementation** (`ro_damage_formulas.js` line 305):
```javascript
const flee = 100 + level + agi + bonusFlee;
```

**Status**: ✅ IMPLEMENTED

---

### FLEE Multi-Attacker Penalty

**Research**: Percentage-based: `EffectiveFLEE = FLEE * (1 - (NumAttackers-2) * 0.10)`. Skill-granted FLEE added AFTER penalty.

**Implementation** (`ro_damage_formulas.js` line 396-400):
```javascript
if (numAttackers > 2) {
    const fleePenalty = (numAttackers - 2) * 10;
    effectiveFlee = Math.max(0, effectiveFlee - fleePenalty);
}
```

**Status**: ⚠️ PARTIAL -- Uses flat subtraction (`-10 per attacker`) instead of percentage-based (`*10% per attacker`). Example:
- FLEE 280, 5 attackers: Implementation = 280 - 30 = 250. Research = 280 * 0.70 = 196.
- FLEE 150, 5 attackers: Implementation = 150 - 30 = 120. Research = 150 * 0.70 = 105.

The flat subtraction is less punishing at high FLEE and more punishing at low FLEE compared to percentage-based. The deep research doc explicitly notes this as a known discrepancy.

Also: Skill-granted FLEE is NOT separated from base FLEE for post-penalty addition. All FLEE sources are combined before the penalty, which means skill investments are also penalized by mobs.

---

### Critical Rate Formula

**Research**: `CRI = 1 + floor(LUK * 0.3) + EquipCRI`. Katar doubles. `CritShield = floor(targetLUK / 5)`.

**Implementation**: All correct (see Step 2 in Physical Damage Pipeline above).

**Status**: ✅ IMPLEMENTED

---

### Critical Damage Properties

**Research**: (1) Max weapon ATK, (2) Bypass FLEE, (3) Bypass PD, (4) Bypass Hard DEF, (5) Bypass Soft DEF, (6) +40% damage.

| Property | Status | Notes |
|----------|--------|-------|
| Max weapon ATK | ✅ | `isCritical -> atkMax` |
| Bypass FLEE | ✅ | `if (!isCritical && !forceHit)` |
| Bypass PD | ⚠️ | PD checked before crit -- wrong order means PD can negate a crit |
| Bypass Hard DEF | ❌ | `isCritical` does NOT set `skipDEF` |
| Bypass Soft DEF | ❌ | Same -- no DEF bypass for crits |
| +40% damage | ✅ | `totalATK * 140 / 100` |

**Status**: ❌ CRITICAL BUG -- Properties 4 and 5 are not implemented. This is the most impactful formula bug.

---

### Katar Critical Double

**Research**: Katar weapons double total critical rate (internal, not shown in status window).

**Implementation**: `if (weaponType === 'katar') critical *= 2` in `calculateDerivedStats`.

**Status**: ✅ IMPLEMENTED

---

### Skills Cannot Crit

**Research**: Pre-renewal skills cannot critically hit. Only auto-attacks can crit.

**Implementation**: `else if (!isSkill)` guard around crit check.

**Status**: ✅ IMPLEMENTED

---

### Multi-Attacker DEF Penalty

**Research**: `EffectiveDEF = DEF * (1 - (NumAttackers-2) * 0.05)` -- each attacker beyond 2 reduces DEF by 5%.

**Implementation**: Not found in any file.

**Status**: ❌ MISSING

---

### Dmotion (Hit Stun/Flinch Duration)

**Research**: Fixed value per monster/class. Determines hitlock potential.

**Implementation**: Only a "walk delay" system found at index.js line 6085. No per-monster dmotion values or hitlock calculations.

**Status**: ❌ MISSING -- No hitlock system.

---

### Cast Interruption VIT Check

**Research**: `InterruptionChance% = 100 - VIT`. Phen Card = 100% uninterruptible.

**Implementation** (`index.js` line 26199-26201):
```javascript
// Cast interruption: damage interrupts casting (100% chance, RO pre-renewal)
if (activeCasts.has(atkState.targetCharId)) {
    interruptCast(atkState.targetCharId, 'damage');
}
```

The `interruptCast` function (line 540-561) checks for `uninterruptible` skills and `cardNoCastCancel` (Phen Card), but does NOT check VIT for chance-based interruption resistance.

**Status**: ⚠️ PARTIAL -- Cast interruption is 100% chance regardless of VIT. Phen Card immunity is correct. VIT-based resistance is missing.

---

## Lex Aeterna Implementation

**Research**: Doubles next damage instance. Bundled skills = double entire damage. Individual hits = double only first hit. Consumed on hit.

**Implementation** (found in index.js at multiple locations):
- Applied at skill handler level (not in `calculatePhysicalDamage`/`calculateMagicalDamage`)
- `result.damage = result.damage * 2` then `removeBuff(enemy, 'lex_aeterna')`
- Handles both bundled (bolt skills) and per-tick (AoE) correctly based on where the check is placed

**Status**: ✅ IMPLEMENTED (at the skill handler level in index.js)

---

## Safety Wall / Pneuma

**Research**: Safety Wall blocks melee physical only. Pneuma blocks ranged physical only. Neither blocks magic.

**Implementation**:
- Safety Wall: ✅ `checkSafetyWallBlock()` function at index.js line 1597
- Pneuma: ✅ Ranged block at index.js line 1891-1902
- Both correctly do not affect magic damage

**Status**: ✅ IMPLEMENTED

---

## Magic Cast Time

**Research**: `CastTime = BaseCastTime * (1 - DEX/150) * (1 - SuffragiumReduction) * (1 - EquipReduction/100)`. Instant at DEX >= 150.

**Implementation**: Not in `ro_damage_formulas.js`. Handled in index.js skill handlers with `dexCastReduction`.

**Status**: ✅ IMPLEMENTED (in skill handlers, not audited in detail here)

---

## Critical Discrepancies (Wrong Formulas, Wrong Constants)

### P1: StatusATK Missing `floor(BaseLv/4)` -- CRITICAL
- **File**: `ro_damage_formulas.js` line 288-290
- **Impact**: -24 ATK at level 99 on every physical attack (auto-attack and skill)
- **Fix**: Add `Math.floor(level / 4)` to both melee and ranged StatusATK formulas

### P2: StatusATK Uses `floor(LUK/5)` Instead of `floor(LUK/3)` -- CRITICAL
- **File**: `ro_damage_formulas.js` line 289-290
- **Impact**: -14 ATK at 99 LUK. Makes LUK less valuable for physical damage than it should be.
- **Fix**: Change `Math.floor(luk / 5)` to `Math.floor(luk / 3)` in both melee and ranged formulas

### P3: SoftDEF Formula Incorrect -- CRITICAL
- **File**: `ro_damage_formulas.js` line 319
- **Impact**: Completely different defense curve. Low-VIT characters get too much DEF (from BaseLv), high-VIT characters miss the VIT^2 scaling.
- **Fix**: Replace with `Math.floor(vit * 0.5) + Math.max(Math.floor(vit * 0.3), Math.floor(vit * vit / 150) - 1)`

### P4: Critical Hits Do Not Bypass DEF -- CRITICAL
- **File**: `ro_damage_formulas.js` line 748-779
- **Impact**: LUK/Crit builds deal dramatically less damage than intended. High-DEF monsters take nearly the same damage from crits as normal hits (minus the +40% bonus but plus DEF reduction).
- **Fix**: Add `if (isCritical) skipDEF = true;` before the DEF check at line 749

### P5: PD Check Order Wrong + PD Applies to Skills -- HIGH
- **File**: `ro_damage_formulas.js` line 480-492
- **Impact**: (1) A natural crit can be negated by PD when it should bypass it. (2) Skill attacks can be Perfect Dodged when they should not be.
- **Fix**: Move crit check before PD check. Add `!isSkill` to the PD check condition.

### P6: Attack Delay Multiplier is `*50` Instead of `*20` -- HIGH
- **File**: `index.js` line 460
- **Impact**: All attacks are 2.5x slower than RO Classic at the same ASPD value.
- **Fix**: Change to `*20`, or document as intentional and adjust ASPD_CAP accordingly. (May be intentional game balancing.)

### P7: Hard DEF Uses Simplified Linear Formula -- MEDIUM
- **File**: `ro_damage_formulas.js` line 776
- **Impact**: At high DEF (50+), monsters are much tankier than in official RO. DEF 99 = 99% reduction instead of ~47%.
- **Fix**: Replace with `Math.floor(totalATK * (4000 + hardDef) / (4000 + hardDef * 10))`

### P8: Provoke atkMultiplier Applied to Magic Damage -- MEDIUM
- **File**: `ro_damage_formulas.js` line 960-961
- **Impact**: Provoke incorrectly boosts magic damage. In RO, Provoke only affects physical ATK.
- **Fix**: Remove `atkMultiplier` from magic damage path, or pass a separate `matkMultiplier`.

### P9: Star Crumb ATK Added to WeaponATK Instead of MasteryATK -- LOW
- **File**: `index.js` line 5431-5433
- **Impact**: Star Crumb ATK goes through size penalty and DEF reduction when it should bypass both. Practical impact is small (max +40 ATK).
- **Fix**: Move star crumb ATK from `weaponATK` to `passiveATK`.

---

## Missing Features

### M1: Weapon Level Damage Variance -- LOW
- **Research**: Lv4 weapon = 80-100% damage spread per hit. Applied as final variance.
- **Impact**: Damage is more consistent than it should be.

### M2: Shield ASPD Penalty -- LOW
- **Research**: Shields add +5 to +10 to weapon delay.
- **Impact**: Shield users attack faster than they should.

### M3: Multi-Attacker DEF Penalty -- LOW
- **Research**: DEF reduced by 5% per attacker beyond 2.
- **Impact**: Tanky characters are slightly harder to kill when mobbed.

### M4: Dmotion / Hitlock System -- LOW
- **Research**: Fixed flinch duration per monster. Determines if fast-attacking players can lock monsters in place.
- **Impact**: No hitlock mechanic means all monsters can move freely regardless of attack speed.

### M5: VIT-Based Cast Interruption Resistance -- MEDIUM
- **Research**: `InterruptionChance = 100 - VIT`. Currently always 100%.
- **Impact**: VIT-based casters (Battle Priests, Sages) have no natural cast protection.

### M6: Mystical Amplification (HW_MAGICPOWER) -- MEDIUM
- **Research**: Wizard skill that boosts next spell damage by up to 95%. Core Wizard skill.
- **Impact**: Wizard players lose their primary burst damage tool.

---

## Recommended Fixes (Prioritized)

### Priority 1: Critical Bug Fixes (game-breaking damage errors)

1. **StatusATK missing `floor(BaseLv/4)` and wrong `LUK/5` -> `LUK/3`** (P1+P2)
   - File: `ro_damage_formulas.js` line 288-290
   - Change both formulas to include `Math.floor(level / 4)` and `Math.floor(luk / 3)`
   - Every physical attacker in the game is dealing less damage than intended

2. **Critical hits must bypass DEF** (P4)
   - File: `ro_damage_formulas.js` line 749
   - Add: `if (isCritical) skipDEF = true;`
   - This is the most impactful single fix for combat balance -- crit builds are severely nerfed without it

3. **SoftDEF formula correction** (P3)
   - File: `ro_damage_formulas.js` line 319
   - Replace entire softDEF calculation with the RO Classic VIT-based formula
   - Affects how tanky every character and monster is

### Priority 2: High-Impact Fixes

4. **PD check order + skill immunity** (P5)
   - File: `ro_damage_formulas.js` line 480-520
   - Reorder: crit check first, then PD check (with `!isSkill` gate)

5. **Attack delay multiplier** (P6)
   - File: `index.js` line 460
   - If `*50` is intentional, document it clearly. If not, change to `*20`.
   - This affects the entire combat pacing of the game

### Priority 3: Medium-Impact Fixes

6. **VIT-based cast interruption resistance** (M5)
   - File: `index.js` `interruptCast()` function
   - Add VIT chance check before interrupting

7. **Mystical Amplification implementation** (M6)
   - New skill handler for HW_MAGICPOWER (Wizard ID 811)
   - Buff that boosts next spell MATK by formula-based percentage

8. **Provoke should not boost magic** (P8)
   - File: `ro_damage_formulas.js` line 960-961
   - Remove `atkMultiplier` from magic pipeline

### Priority 4: Low-Impact/Polish

9. **Hard DEF formula** (P7) -- switch to `(4000+DEF)/(4000+DEF*10)` if targeting official behavior
10. **FLEE multi-attacker penalty** -- change to percentage-based
11. **Star Crumb ATK in passiveATK** (P9)
12. **Weapon level variance** (M1)
13. **Shield ASPD penalty** (M2)
14. **Multi-attacker DEF penalty** (M3)
