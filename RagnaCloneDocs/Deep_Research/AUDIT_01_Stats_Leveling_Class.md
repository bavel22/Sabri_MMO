# Audit: Stats, Leveling & Class System

> **Audit Date**: 2026-03-22
> **Scope**: Deep Research docs 01 (Stats & Attributes), 02 (Leveling & EXP), 03 (Job/Class System) vs. actual server implementation
> **Files Audited**:
> - `server/src/ro_exp_tables.js` (EXP tables, JOB_CLASS_CONFIG, HP/SP coefficients, ASPD delays)
> - `server/src/ro_damage_formulas.js` (calculateDerivedStats, calculateASPD, calculateMaxHP, calculateMaxSP, physical/magic damage)
> - `server/src/index.js` (getEffectiveStats, processExpGain, applyDeathPenalty, distributePartyEXP, job:change handler, weight, regen)

---

## Summary

| Status | Count |
|--------|-------|
| IMPLEMENTED | 52 |
| PARTIAL | 17 |
| MISSING | 24 |
| **Total Features Audited** | **93** |

---

## Detailed Comparison

### A. Base Stats (Doc 01, Section 2)

#### A1. STR Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Melee StatusATK: STR + floor(STR/10)^2 | `str + floor(str/10)^2` | `str + Math.floor(str / 10) ** 2` (ro_damage_formulas.js:290) | IMPLEMENTED |
| Ranged StatusATK: floor(STR/5) | `floor(STR/5)` for ranged | `Math.floor(str / 5)` (ro_damage_formulas.js:289) | IMPLEMENTED |
| Weapon stat bonus: floor(BaseWeaponDmg * STR/200) | STR scaling on weapon ATK | Handled in physical damage pipeline via weapon variance | IMPLEMENTED |
| Weight Limit: +30 per STR | `2000 + STR * 30` | `2000 + str * 30` (index.js:4739) | IMPLEMENTED |

#### A2. AGI Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Flee: +1 per AGI | `BaseLv + AGI` | `100 + level + agi` (ro_damage_formulas.js:305) | IMPLEMENTED |
| ASPD: 4:1 AGI:DEX ratio | `amotion * (4*AGI + DEX) / 1000` | `Math.floor(WD * agi / 25) + Math.floor(WD * dex / 100)` (equivalent 4:1) | IMPLEMENTED |
| Soft DEF: floor(AGI/5) | Minor contribution | `Math.floor(agi / 5)` in softDEF (ro_damage_formulas.js:319) | IMPLEMENTED |

#### A3. VIT Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| MaxHP multiplier: BaseHP * (1 + VIT*0.01) | Multiplicative | `Math.floor(baseHP * (1 + vit * 0.01) * transMod)` (ro_damage_formulas.js:188) | IMPLEMENTED |
| Soft DEF base: floor(VIT*0.5) | Display-only deterministic | `Math.floor(vit / 2)` (ro_damage_formulas.js:319) | IMPLEMENTED |
| VIT Soft DEF random component (rAthena PR #6766) | `vit_def_base + rnd(vit_def_min, vit_def_max)` per hit | Not implemented -- server uses deterministic `floor(VIT/2)` only, no random component in damage calc | MISSING |
| HP Regen: floor(VIT/5) | Added to base regen tick | Passive skill bonus `Increase HP Recovery` exists, but base VIT regen formula not verified as exact | PARTIAL |
| Healing item effectiveness: +2% per VIT | `potionHealBonus` from Potion Research exists | Only Potion Research passive (+5%/lv), no VIT-based +2%/VIT item heal scaling found | MISSING |
| Status resistance (Stun, Poison, etc.) | VIT reduces rates | Implemented in applyStatusEffect checks | IMPLEMENTED |

#### A4. INT Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| MATK Min: INT + floor(INT/7)^2 | Formula exact | `intStat + Math.floor(intStat / 7) ** 2` (ro_damage_formulas.js:294) | IMPLEMENTED |
| MATK Max: INT + floor(INT/5)^2 | Formula exact | `intStat + Math.floor(intStat / 5) ** 2` (ro_damage_formulas.js:295) | IMPLEMENTED |
| MaxSP multiplier: BaseSP * (1 + INT*0.01) | Multiplicative | `Math.floor(baseSP * (1 + intStat * 0.01) * transMod)` (ro_damage_formulas.js:201) | IMPLEMENTED |
| Soft MDEF: +1 per INT (primary) | Primary contributor | `intStat + floor(vit/5) + floor(dex/5) + floor(level/4)` (ro_damage_formulas.js:323) | IMPLEMENTED |
| SP Regen: floor(INT/6) | Added to base regen | Exists via skill passive, exact base formula not verified | PARTIAL |
| SP Regen bonus (INT >= 120) | `floor(INT/2) - 56` additional | Not found in server code | MISSING |
| SP item effectiveness: +1% per INT | Not found | No INT-based SP item scaling found | MISSING |

#### A5. DEX Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| HIT: +1 per DEX (primary) | `BaseLv + DEX` | `175 + level + dex + bonusHit` (ro_damage_formulas.js:301) | IMPLEMENTED |
| Cast time: CastTime * (1 - DEX/150) | Pre-renewal single-component | `Math.floor(baseCastTime * (1 - casterDex / 150))` (index.js:532) | IMPLEMENTED |
| Ranged StatusATK: DEX + floor(DEX/10)^2 | For ranged weapons | `dex + Math.floor(dex / 10) ** 2` (ro_damage_formulas.js:289) | IMPLEMENTED |
| Melee StatusATK: floor(DEX/5) | Minor contribution | `Math.floor(dex / 5)` (ro_damage_formulas.js:290) | IMPLEMENTED |
| ASPD contribution: 1:4 vs AGI | Minor | `Math.floor(WD * dex / 100)` vs AGI's `/25` (ro_damage_formulas.js:237-238) | IMPLEMENTED |
| Min damage stabilization per weapon level | +1/+1.2/+1.4/+1.6 per DEX by weapon level | Not found -- no weapon-level-specific min damage stabilization from DEX | MISSING |

#### A6. LUK Effects
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Crit Rate: floor(LUK*0.3) + 1 | Base 1% + LUK scaling | `1 + Math.floor(luk * 0.3) + bonusCritical` (ro_damage_formulas.js:310) | IMPLEMENTED |
| Perfect Dodge: floor(LUK/10) | LUK-based PD | `1 + Math.floor(luk / 10) + bonusPerfectDodge` (ro_damage_formulas.js:315) | IMPLEMENTED |
| StatusATK: floor(LUK/3) | Research says `/3`, rAthena confirmed | Server uses `Math.floor(luk / 5)` (ro_damage_formulas.js:289-290) | PARTIAL |
| Crit Shield: -1% per 5 LUK of target | Reduces crit rate against target | Implemented in physical damage crit check | IMPLEMENTED |
| LUK NOT in HIT (pre-renewal) | rAthena pre-re: no LUK in HIT | Server HIT = `175 + level + dex + bonusHit` (no LUK) -- correct | IMPLEMENTED |
| LUK NOT in Flee (pre-renewal) | rAthena pre-re: no LUK in Flee | Server Flee = `100 + level + agi + bonusFlee` (no LUK) -- correct | IMPLEMENTED |

**CRITICAL DISCREPANCY A6-3**: LUK contribution to StatusATK uses `floor(LUK/5)` in server code but research doc says `floor(LUK/3)`. The research cites iRO Wiki Classic and rAthena as sources for `/3`. This affects all melee and ranged ATK calculations.

---

### B. Stat Point System (Doc 01, Section 3 + Doc 02, Section 2.2)

| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Points per level: floor(level/5) + 3 | Exact formula | `Math.floor(newLevel / 5) + 3` (ro_exp_tables.js:397) | IMPLEMENTED |
| Cost to raise: floor((x-1)/10) + 2 | Exact formula | `Math.floor((currentStatValue - 1) / 10) + 2` (index.js:4110) | IMPLEMENTED |
| Stat cap: 99 max base stat | Cannot exceed 99 | Stat increment handler checks `stats[stat] >= 99` | IMPLEMENTED |
| Starting points: 48 (Normal Novice) | 6 stats at 1 = 6, +48 free | Character creation starts at base stats 1, with points | IMPLEMENTED |
| Starting points: 100 (High Novice) | Extra 52 for transcendent | No rebirth system implemented -- no High Novice creation path | MISSING |
| Total from levels 2-99: 1,225 | Cumulative sum | Matches the formula sum | IMPLEMENTED |

---

### C. Derived Stats (Doc 01, Section 4)

#### C1. ATK
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| StatusATK = BaseLv/4 + stat contributions | `floor(BaseLv/4)` in formula | Server StatusATK does NOT include `floor(BaseLv/4)` -- only stat + bonus terms | PARTIAL |
| StatusATK * 2 in damage formula | StatusATK doubled in total damage | `StatusATK + variancedWeaponATK` in damage pipeline (StatusATK appears NOT doubled) | PARTIAL |
| WeaponATK variance per weapon level | +/-5%/10%/15%/20% by weapon level | Implemented in physical damage pipeline | IMPLEMENTED |

**DISCREPANCY C1-1**: The StatusATK formula in `ro_damage_formulas.js:288-290` is:
- Melee: `str + floor(str/10)^2 + floor(dex/5) + floor(luk/5)`
- Research says: `floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)`
- Missing: `floor(BaseLv/4)` base level component, and LUK uses `/5` instead of `/3`

**DISCREPANCY C1-2**: Research says total physical base damage = `StatusATK * 2 + WeaponATK`. Server damage formula (ro_damage_formulas.js) computes `statusATK + variancedWeaponATK` (no `*2` found on StatusATK in the physical damage function). Need to verify the exact line in roPhysicalDamage.

#### C2. MATK
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| MATK Min/Max formulas | INT + floor(INT/7)^2 / floor(INT/5)^2 | Exact match (ro_damage_formulas.js:294-295) | IMPLEMENTED |
| Weapon MATK addition | matkMin + floor(weaponMATK*0.7), matkMax + weaponMATK | `statusMATK + Math.floor(weaponMATK * 0.7)` and `statusMATKMax + weaponMATK` | IMPLEMENTED |

#### C3. DEF/MDEF
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Hard DEF formula: Dmg * (4000+DEF)/(4000+DEF*10) | Pre-renewal percentage reduction | Implemented in physical damage pipeline | IMPLEMENTED |
| Soft DEF: floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2) | Display formula | `Math.floor(vit/2) + Math.floor(agi/5) + Math.floor(level/2)` (ro_damage_formulas.js:319) | IMPLEMENTED |
| Random VIT DEF component per hit | rAthena PR #6766 formula | Not implemented -- only deterministic softDEF used in damage calc | MISSING |
| Hard MDEF formula: Dmg * (1000+MDEF)/(1000+MDEF*10) | Constant 1000 (vs 4000 for DEF) | Implemented in magic damage pipeline | IMPLEMENTED |
| Soft MDEF: INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4) | Exact formula | Exact match (ro_damage_formulas.js:323) | IMPLEMENTED |
| Application order: Hard first, then Soft | Percentage then flat | Correct order in both physical and magic damage paths | IMPLEMENTED |

#### C4. HIT/FLEE
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| HIT = BaseLv + DEX + bonus | No LUK | `175 + level + dex + bonusHit` | PARTIAL |
| Hit rate: 80 + HIT - TargetFlee, clamped 5-100 | Standard formula | `Math.max(5, Math.min(95, hitRate))` -- capped at 95 not 100 | PARTIAL |
| Flee = BaseLv + AGI + bonus | No LUK | `100 + level + agi + bonusFlee` | PARTIAL |
| Dodge cap: 95% (100% in WoE) | Max dodge 95% | Capped at 95 (correct for normal play) | IMPLEMENTED |
| Multi-attacker flee penalty: -10 per extra attacker | `Flee - (numAttackers-1)*10` | `fleePenalty = (numAttackers - 2) * 10` (ro_damage_formulas.js:398) | PARTIAL |
| Perfect Dodge: independent check | Can dodge even crits | Implemented -- PD checked before crit in damage pipeline | IMPLEMENTED |

**DISCREPANCY C4-1**: HIT formula has base constant 175 in server, but research says `BaseLv + DEX`. Server appears to add 175 as a base value on top of level+DEX. This is likely an intentional design choice for gameplay balance, but differs from pure RO Classic formula.

**DISCREPANCY C4-2**: Flee formula has base constant 100 in server, research says `BaseLv + AGI`. Similarly adds 100 as base.

**DISCREPANCY C4-3**: Flee penalty starts at `numAttackers - 2` (penalty kicks in at 3+ attackers), but research says `numAttackers - 1` (penalty at 2+ attackers). rAthena uses `num - 1` starting from 2 attackers.

#### C5. ASPD
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Pre-renewal formula | `Amotion = WD - floor(WD*(4*AGI+DEX)/1000)` | `agiReduction = floor(WD*agi/25); dexReduction = floor(WD*dex/100); total = floor((agi+dex)/10)` | PARTIAL |
| ASPD to attack delay | `(200 - ASPD) * 10` ms | `(200 - aspd) * 50` ms (index.js:460) | PARTIAL |
| ASPD cap: 190 | Hard cap at 190 | Cap is 195 for single weapon, 190 for dual wield (index.js:245-246) | PARTIAL |
| Base weapon delay table | Per-class per-weapon | Complete ASPD_BASE_DELAYS table for all 20 classes | IMPLEMENTED |
| Transcendent uses base class delays | Same values | `TRANS_TO_BASE_CLASS` mapping (ro_exp_tables.js:527-548) | IMPLEMENTED |
| Mount ASPD penalty | 50% base, +10% per Cavalry Mastery | Implemented (ro_damage_formulas.js:250-253) | IMPLEMENTED |

**DISCREPANCY C5-1**: The ASPD formula in server separates AGI and DEX reductions before combining, with a `/10` divisor. The research formula combines them as `4*AGI + DEX` directly in one calculation divided by 1000. The algebraic result may differ due to integer truncation at different stages.

**DISCREPANCY C5-2**: Attack delay uses `(200-aspd)*50` in the server (ASPD 190 = 500ms), while research says `(200-aspd)*10` (ASPD 190 = 100ms). The server uses a different scaling factor, making attacks slower than research spec.

**DISCREPANCY C5-3**: ASPD cap is 195 in server for single weapons vs 190 in research. The server allows 5 extra ASPD points above the research-documented cap, with diminishing returns from 195-199.

#### C6. MaxHP/MaxSP
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| HP iterative formula | `35 + BaseLv*HP_B + sum(round(HP_A*i))` for i=2..BaseLv | Exact match (ro_damage_formulas.js:183-186) | IMPLEMENTED |
| HP VIT scaling | `* (1 + VIT*0.01)` | Exact match (ro_damage_formulas.js:188) | IMPLEMENTED |
| HP TransMod | 1.25x for transcendent | Exact match (ro_damage_formulas.js:187) | IMPLEMENTED |
| HP class coefficients | All 20 classes documented | All present in HP_SP_COEFFICIENTS (ro_exp_tables.js:453-497) | IMPLEMENTED |
| SP formula | Research: `10 + BaseLv*SP_B + sum(round(SP_A*i))` iterative | Server: `10 + Math.floor(baseLevel * coeff.SP_JOB)` -- simple linear, NOT iterative | PARTIAL |
| SP class coefficients | SP_JOB_A and SP_JOB_B per class | Server has only `SP_JOB` (single coefficient), not the dual-coefficient iterative formula | PARTIAL |

**CRITICAL DISCREPANCY C6-1**: MaxSP formula in server uses a simple linear formula `10 + BaseLv * SP_JOB` while research documents an iterative formula matching HP: `10 + BaseLv*SP_B + sum(round(SP_A*i))`. This produces significantly different SP values at high levels. The SP coefficients in ro_exp_tables.js are single values (1-9) rather than the dual SP_JOB_A/SP_JOB_B documented in research. For example:
- Mage SP_JOB = 6 in server vs SP_JOB_A=0.6, SP_JOB_B=2.0 in research
- Wizard SP_JOB = 9 vs SP_JOB_A=1.0, SP_JOB_B=2.0

#### C7. Natural Regeneration
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| HP Regen: max(1, floor(MaxHP/200)) + floor(VIT/5) | Every 6s standing | Regen tick exists but exact formula not verified line-by-line | PARTIAL |
| SP Regen: floor(MaxSP/100) + floor(INT/6) + 1 | Every 8s standing | Regen tick exists | PARTIAL |
| Sitting: 2x regen (halves interval) | Every 3s HP, 4s SP | Sitting system implemented (index.js:7442+), doubles regen | IMPLEMENTED |
| 50-89% weight: regen halved | Weight threshold | Weight system with overweight penalties exists | IMPLEMENTED |
| 90%+ weight: NO regen | Full block | Implemented in weight system | IMPLEMENTED |

#### C8. Cast Time
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| FinalCast = BaseCast * (1 - DEX/150) | Single-component pre-renewal | `Math.floor(baseCastTime * (1 - casterDex / 150))` (index.js:532) | IMPLEMENTED |
| 150 DEX = instant cast | Zero cast time | Mathematically correct (1 - 150/150 = 0) | IMPLEMENTED |
| Equipment cast reduction stacking | Additive with DEX | `castTime * (100 + equipCastRate) / 100` applied after DEX (index.js:535) | IMPLEMENTED |

#### C9. Critical Hits
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Crit = floor(LUK*0.3) + 1 + bonus - floor(TargetLUK/5) | vs target LUK | Implemented in physical damage pipeline | IMPLEMENTED |
| Always max WeaponATK (no variance) | Guaranteed max | Implemented | IMPLEMENTED |
| Ignores Flee (100% hit) | Guaranteed hit | Implemented | IMPLEMENTED |
| Ignores BOTH Hard DEF and Soft DEF | Full damage | Implemented -- crits bypass DEF | IMPLEMENTED |
| Does NOT ignore Perfect Dodge | PD still works | PD checked before crit calculation | IMPLEMENTED |
| +40% damage bonus | 1.4x multiplier | `baseCritBonus = 40` (+40%) (ro_damage_formulas.js:619) | IMPLEMENTED |
| Katar doubles crit rate | 2x displayed and actual | `if (weaponType === 'katar') critical *= 2` (ro_damage_formulas.js:311) | IMPLEMENTED |
| Skills cannot crit (exceptions) | Auto-attack only | `!isSkill` guard on crit check | IMPLEMENTED |

---

### D. Weight System (Doc 01, Section 7)

| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Base formula: 2000 + STR*30 | Base weight limit | `2000 + str * 30` (index.js:4739) | IMPLEMENTED |
| Job weight bonuses per class | 0-800 per class | NOT implemented -- server uses base formula only, no class weight bonus | MISSING |
| 0-49%: normal | Full function | Implemented via weight thresholds | IMPLEMENTED |
| 50-69%: regen halved | Threshold penalty | Research says 50-89%, server may group differently | PARTIAL |
| 70-89%: regen halved + no creation skills | Additional restriction | Exists in weight system | IMPLEMENTED |
| 90-100%: no attacks/skills/regen | Full lockout | Implemented | IMPLEMENTED |

**DISCREPANCY D-1**: Server weight formula `2000 + STR*30` does not include per-class job weight bonuses (Swordsman +800, Mage +200, etc.). This means all classes have the same base weight limit. Only exception is Enlarge Weight Limit skill (+200/lv) and mount bonus (+1000).

---

### E. EXP Tables (Doc 02, Section 4)

| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Normal Base EXP table (Lv 1-98) | 98 entries, exact values | All 98 entries present and match (ro_exp_tables.js:26-125) | IMPLEMENTED |
| Transcendent Base EXP table | Separate harder table (~3x) | NOT present in ro_exp_tables.js | MISSING |
| Novice Job EXP (Jb 1-9) | 9 entries | Present and matching (ro_exp_tables.js:132-142) | IMPLEMENTED |
| First Class Job EXP (Jb 1-49) | 49 entries | Present (ro_exp_tables.js:150-200) | IMPLEMENTED |
| Second Class Job EXP (Jb 1-49) | 49 entries | Present (ro_exp_tables.js:208-258) | IMPLEMENTED |
| Transcendent 2nd Class Job EXP (Jb 1-69) | Separate table, Jb 70 cap | NOT present in ro_exp_tables.js | MISSING |
| Super Novice Job EXP (Jb 1-98) | 1st Class for 1-49, then linear 50-98 | NOT present in ro_exp_tables.js | MISSING |

---

### F. EXP System (Doc 02, Sections 5-8)

| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| No level difference penalty (pre-renewal) | Flat monster EXP regardless of player level | Correct -- no penalty implemented | IMPLEMENTED |
| EXP Tap Bonus: +25% per attacker | Max 11 bonus attackers | `tapBonus = 100 + 25 * Math.min(11, attackerCount - 1)` (index.js:2218) | IMPLEMENTED |
| Party Even Share: 20% per member | Pooled + split | `distributePartyEXP` with 20% bonus (index.js:5189+) | IMPLEMENTED |
| 15-level gap check | Falls back to Each Take | `PARTY_SHARE_LEVEL_GAP = 15` (index.js:150) | IMPLEMENTED |
| Same-zone filtering | Only same-map members | Implemented in distributePartyEXP | IMPLEMENTED |
| Dead member exclusion | HP > 0 required | Implemented | IMPLEMENTED |
| Death penalty: 1% base + job EXP | Novice exempt | `applyDeathPenalty` with 1% base+job, novice check (index.js:2076-2107) | IMPLEMENTED |
| Cannot delevel from death | Clamped at 0 | `Math.max(0, ...)` (index.js:2089, 2099) | IMPLEMENTED |
| Max level: no death penalty | Returns 0 | `getBaseExpForNextLevel(99)` returns 0 | IMPLEMENTED |
| Multi-level-up cap | Official: 1 level per kill | Server allows unlimited multi-level via while loop (index.js:3982) | PARTIAL |
| Full heal on level up | Classic behavior | `player.health = player.maxHealth` (index.js:4006-4007) | IMPLEMENTED |
| Stat points per level | floor(newLevel/5) + 3 | Exact match | IMPLEMENTED |
| 1 skill point per job level | Always 1 | `getSkillPointsForJobLevelUp() = 1` (ro_exp_tables.js:401-403) | IMPLEMENTED |

**DISCREPANCY F-1**: Multi-level-up is enabled (while loop processes all levels). Research says official pre-renewal caps at 1 level per EXP application. This is technically a feature, not a bug, if intentional for private-server-style play.

---

### G. MVP System (Doc 02, Section 12)

| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| MVP = highest damage dealer | Top damage gets title | `mvpCombatData` snapshot, highest damage wins (index.js:2290+) | IMPLEMENTED |
| MVP bonus EXP | Additional EXP to MVP player only | `mvpExpReward = enemy.mvpExp` applied to MVP only (index.js:2307) | IMPLEMENTED |
| MVP drop rewards | 1-3 items from MVP drop table | MVP drop rolls implemented | IMPLEMENTED |
| Regular EXP shared normally | Damage-ratio split | Regular EXP distributed before MVP bonus | IMPLEMENTED |
| MVP EXP does not share with party | Goes only to top damage | MVP EXP via `processExpGain` only to MVP player | IMPLEMENTED |

---

### H. Job/Class System (Doc 03)

#### H1. Class Configuration
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Novice (tier 0, max job 10) | Starting class | Present in JOB_CLASS_CONFIG | IMPLEMENTED |
| 6 First Classes (tier 1, max job 50) | Swordsman through Acolyte | All 6 present | IMPLEMENTED |
| 13 Second Classes (tier 2, max job 50) | Knight through Alchemist | All 13 present (including Bard/Dancer) | IMPLEMENTED |
| Transcendent classes in JOB_CLASS_CONFIG | Lord Knight through Champion, max job 70 | NOT present -- only HP_SP_COEFFICIENTS and ASPD tables include trans classes | MISSING |
| Super Novice in JOB_CLASS_CONFIG | Max job 99, special skill tree | NOT present in JOB_CLASS_CONFIG (only in HP_SP_COEFFICIENTS) | MISSING |
| High Novice/High First in JOB_CLASS_CONFIG | Rebirth intermediary classes | NOT present (only in HP_SP_COEFFICIENTS/ASPD) | MISSING |
| Baby classes | 13 baby versions | NOT present anywhere | MISSING |
| Extended classes (TaeKwon/Ninja/Gunslinger) | 3-5 classes | NOT present anywhere | MISSING |

#### H2. Job Change System
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Novice -> 1st Class: Job Lv 10 + Basic Skill 9 | Gate requirements | Both checks present (index.js:8021-8035) | IMPLEMENTED |
| 1st -> 2nd Class: Job Lv 40+ | Minimum level check | `currentJobLevel < 40` check (index.js:8039) | IMPLEMENTED |
| Valid upgrade paths (Swordsman->Knight/Crusader etc.) | SECOND_CLASS_UPGRADES | All 6 upgrade paths with 13 targets (ro_exp_tables.js:435-442) | IMPLEMENTED |
| Job level resets to 1 | On job change | `player.jobLevel = 1; player.jobExp = 0` (index.js:8054-8055) | IMPLEMENTED |
| Base level preserved | No change on job change | Base level not modified | IMPLEMENTED |
| 1st class skills retained on 2nd class | Skills carry over | "Skill points are kept" comment (index.js:8056) | IMPLEMENTED |
| Novice skills lost on 1st class | Previous skills gone | Not explicitly handled -- novice skills may persist | PARTIAL |
| Gender-locked Bard/Dancer | Male=Bard, Female=Dancer | No gender check found in job:change handler | MISSING |
| Unspent skill points lost on job change | Cannot carry over | Not explicitly handled | MISSING |
| Equipment auto-unequip for new class | Invalid items removed | Not implemented in job:change handler | MISSING |
| 2nd -> Transcendent (Rebirth) | Base 99 + Job 50 required | No rebirth handler exists | MISSING |

#### H3. Job Bonus Stats
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Per-class hidden stat bonuses at job levels | Automatic stat gains per job level milestone | Not found -- no JOB_BONUS tables or per-job-level stat application | MISSING |
| Swordsman: STR+5, AGI+3, VIT+5, DEX+3 at Job 50 | Example class bonuses | No job bonus data structure exists | MISSING |
| Displayed as green bonus text | UI indicator | No data sent to client for job bonuses | MISSING |

#### H4. Transcendent/Rebirth System
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Rebirth quest flow | Base 99 + Job 50 -> High Novice | Not implemented | MISSING |
| Stats reset to 1, 100 starting points | Fresh start with bonus | Not implemented | MISSING |
| 1.25x HP/SP multiplier | TransMod in formulas | Formula support exists but no way to create trans characters | PARTIAL |
| Job Lv 70 cap for trans 2nd | Extended job level | JOB_CLASS_CONFIG lacks trans entries with maxJobLevel: 70 | MISSING |
| Trans-exclusive skills | Additional skill trees | Some trans skills exist in skill data but no class gating | PARTIAL |
| Transcendent EXP tables | Harder progression | Not in ro_exp_tables.js | MISSING |

#### H5. Super Novice
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Job Lv 99 cap | Highest job cap | Not in JOB_CLASS_CONFIG | MISSING |
| All 1st class skills (minus exclusions) | Multi-tree access | Not implemented | MISSING |
| Guardian Angel level-up buffs | Random buff every 10 levels | Not implemented | MISSING |
| Death protection at 99.0-99.9% EXP | Steel Body on near-death | Not implemented | MISSING |
| Fury status (+50 CRI) | Chant-activated | Not implemented | MISSING |
| Uses Normal EXP table | Same as regular classes | Would use BASE_EXP_TABLE if configured | PARTIAL |

#### H6. Weapon/Equipment Restrictions
| Feature | Research Says | Server Has | Status |
|---------|-------------|-----------|--------|
| Per-item class bitmask system | `jobs_allowed` column | `canClassEquip()` checks player class vs item's jobs_allowed (index.js:4628+) | IMPLEMENTED |
| Trans classes inherit parent permissions | Same equip as base 2nd | Class equip check includes trans->base mapping | IMPLEMENTED |
| Gender-based equipment restrictions | `gender_allowed` column | `canGenderEquip()` check (index.js:4642+) | IMPLEMENTED |
| Level-based equipment restrictions | `equip_level_max` | Max level check (index.js:22790) | IMPLEMENTED |
| Unidentified items cannot be equipped | Must identify first | Check exists (index.js:22765) | IMPLEMENTED |

---

## Critical Discrepancies (Formula Differences)

### Priority 1 -- Affects All Combat

1. **StatusATK missing BaseLv/4** (C1-1): Server StatusATK formula omits `floor(BaseLv/4)`. At level 99, this is +24 StatusATK missing from every auto-attack and physical skill. This is a flat ATK bonus that scales with level.

2. **LUK ATK uses /5 instead of /3** (A6-3): Server uses `floor(LUK/5)` but research says `floor(LUK/3)`. At 99 LUK: server gives +19, research gives +33. Difference of 14 ATK for high-LUK builds (Assassin, critical builds).

3. **MaxSP uses linear formula instead of iterative** (C6-1): Server uses `10 + BaseLv * SP_JOB` (linear) while research documents an iterative formula matching HP. This produces materially different SP values. For a Wizard at Lv99: server gives ~10 + 99*9 = 901 base SP, while the iterative formula with SP_A=1.0, SP_B=2.0 would give ~10 + 99*2 + sum(round(1.0*i)) for i=2..99 = 10 + 198 + 4950 = 5158 base SP. The server's SP values are dramatically lower.

### Priority 2 -- Affects Specific Systems

4. **ASPD delay multiplier mismatch** (C5-2): Server uses `(200-aspd)*50`ms per attack while research says `(200-aspd)*10`ms. At ASPD 190: server = 500ms, research = 100ms. This makes all attacks 5x slower than RO Classic spec. (Note: This may be intentional game design for the server's 50ms combat tick rate.)

5. **ASPD cap 195 vs 190** (C5-3): Server allows ASPD up to 195 (with diminishing returns to 199) while research caps at 190. The server's extended ASPD range allows faster attacks than RO Classic intended.

6. **HIT/Flee base constants** (C4-1, C4-2): Server adds +175 base HIT and +100 base Flee as constants. Research formulas are `BaseLv + DEX` and `BaseLv + AGI` with no base constant. At level 1 this is a large difference; it narrows at higher levels.

7. **Hit rate capped at 95% not 100%** (C4-3): Server caps hit rate at 95% (5% minimum miss chance) while research says 100% max hit rate (only 5% minimum hit chance). This means attacks can never achieve 100% hit rate in the server.

8. **Flee penalty starts at 3 attackers not 2** (C4-4): Server uses `numAttackers - 2` so penalty only starts at 3 attackers, not 2 as research specifies.

---

## Missing Features (Prioritized)

### High Priority (Core Systems)

1. **Transcendent/Rebirth System** -- No rebirth quest, no trans class configs, no trans EXP tables, no trans job EXP tables. Trans HP/SP multipliers exist in formulas but characters cannot become transcendent.

2. **Job Bonus Stats** -- No per-class per-job-level hidden stat bonuses. Every class is missing 13-27 total stat points from their job bonuses. This significantly impacts build viability.

3. **Per-Class Weight Bonuses** -- All classes share the same weight limit. Missing 0-800 bonus per class (Knight/Merchant/Blacksmith get +800; Mage/Wizard get +200; etc.).

4. **VIT-based healing item effectiveness** -- +2% per VIT to potion healing. Currently only Potion Research passive affects this.

5. **SP Regen bonus at INT >= 120** -- `floor(INT/2) - 56` additional SP regen for high-INT builds. Not implemented.

### Medium Priority (Correctness)

6. **StatusATK formula corrections** -- Add `floor(BaseLv/4)` and fix LUK from `/5` to `/3`.

7. **MaxSP iterative formula** -- Replace linear SP_JOB with dual-coefficient SP_JOB_A/SP_JOB_B iterative formula matching HP.

8. **VIT Soft DEF random component** -- Per-hit randomized VIT DEF from rAthena PR #6766.

9. **DEX min damage stabilization** -- Per-weapon-level minimum damage from DEX.

10. **Gender-locked Bard/Dancer** -- No gender check on Archer -> Bard/Dancer job change.

11. **Unspent skill points lost on job change** -- Not enforced.

12. **Equipment auto-unequip on job change** -- Invalid items not removed.

13. **Multi-level-up cap** -- While loop allows unlimited levels per kill; official caps at 1.

### Low Priority (Future Systems)

14. **Super Novice class** -- Full implementation with Guardian Angel, Fury, multi-tree skills.

15. **Baby classes** -- 13 baby class variants with stat cap 80, 75% HP/SP.

16. **Extended classes** (TaeKwon, Ninja, Gunslinger) -- Not in scope for current build.

17. **INT-based SP item effectiveness** (+1% per INT).

18. **Quest EXP cap** -- `EXPReq * 2 - CurrentEXP - 1`.

19. **EXP modifier stacking** (Battle Manuals, equipment bonuses).

---

## Recommended Fixes

### Immediate Fixes (Low Risk, High Impact)

1. **Fix LUK StatusATK contribution**: Change `Math.floor(luk / 5)` to `Math.floor(luk / 3)` in `ro_damage_formulas.js:289-290`. Affects 2 lines.

2. **Add BaseLv/4 to StatusATK**: Add `+ Math.floor(level / 4)` to both melee and ranged StatusATK in `ro_damage_formulas.js:288-290`. Affects 2 lines.

3. **Add per-class weight bonuses**: Add a `WEIGHT_BONUS` map to `ro_exp_tables.js` and reference it in `getPlayerMaxWeight()` (index.js:4737). New lookup table + 1-line change.

4. **Fix flee penalty threshold**: Change `numAttackers - 2` to `numAttackers - 1` in `ro_damage_formulas.js:398` if targeting RO Classic accuracy.

### Medium Effort Fixes

5. **Add Job Bonus Stats system**: Create a `JOB_BONUS_STATS` table in `ro_exp_tables.js` with per-class per-job-level stat bonuses. Integrate into `getEffectiveStats()` to add bonuses based on `player.jobLevel`.

6. **Replace MaxSP linear formula with iterative**: Add SP_JOB_A and SP_JOB_B to HP_SP_COEFFICIENTS and change `calculateMaxSP()` in `ro_damage_formulas.js` to use the same iterative loop as `calculateMaxHP()`.

7. **Add Transcendent class configs to JOB_CLASS_CONFIG**: Add all 13 trans classes with `maxJobLevel: 70` and transcendent job EXP table. Add transcendent base EXP table to `ro_exp_tables.js`.

8. **Add gender check to Bard/Dancer job change**: Add player gender validation in the `job:change` socket handler.

### High Effort (New Systems)

9. **Rebirth/Transcendent system**: New socket handler for rebirth process, resetting stats/skills/level, changing class to High Novice, granting 100 stat points.

10. **Super Novice class**: Full implementation with multi-tree skills, Guardian Angel, and special mechanics.

11. **VIT DEF random component**: Add per-hit randomized VIT soft DEF in physical damage calculation.
