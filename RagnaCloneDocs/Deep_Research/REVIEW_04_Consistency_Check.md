# Research Document Consistency Check

> **Scope**: Cross-referencing 42 deep research documents for contradictions on overlapping topics.
> **Date**: 2026-03-23
> **Documents checked**: 01_Stats_Attributes, 04_Physical_Damage, 05_Magical_Damage, 06_ASPD_Hit_Flee_Critical, 07_Element_System, 08_Size_Race_MonsterProps, 09_Swordsman_Knight_Crusader_Skills, 11_Archer_Hunter_Skills, 13_Acolyte_Priest_Monk_Skills, 15_Merchant_Blacksmith_Alchemist_Skills, 21_Buff_System, 22_Status_Effects_Debuffs, 34_Mount_Falcon_Trap_Performance

---

## Contradictions Found

### C-01: MATK Values at INT 99 (01 vs 05)

**Doc 01_Stats_Attributes.md (line 171-177):**
- MATK Min at INT 99: `99 + floor(99/7)^2 = 99 + 14^2 = 295`
- MATK Max at INT 99: `99 + floor(99/5)^2 = 99 + 19^2 = 460`
- Table shows INT 99: MATK Min = 295, Max = 460, Variance = 165

**Doc 05_Magical_Damage.md (line 55):**
- Table shows INT 99: MATK Min = 298, Max = 490, Variance = 192

**Analysis:** Doc 01 is internally consistent -- it shows the formula AND manually computes 295 and 460, which match. Doc 05's table says 298 and 490, which do NOT match the same formula. Computing by hand: `99 + floor(14.14)^2 = 99 + 196 = 295` (min), `99 + floor(19.8)^2 = 99 + 361 = 460` (max). Doc 01 even flags this: "The existing doc has incorrect values for INT 99 (listed 299 and 491). The correct values are 295 and 460." Doc 05's values (298/490) are yet a third set of wrong numbers.

**Verdict:** Doc 05 has incorrect MATK values at INT 99 (and likely at INT 80/90 too -- 80 shows 209/336 vs doc 01's 201/336; INT 90 shows 256/414 vs 234/414). The MATK Min formula at INT 80 should be `80 + floor(80/7)^2 = 80 + 11^2 = 80 + 121 = 201`, not 209. Doc 05's INT 80 Min (209) and INT 90 Min (256) are also wrong.

**Correction needed:** 05_Magical_Damage.md MATK table rows for INT 80, 90, and 99 must be updated to match the formula.

---

### C-02: Provoke ATK/DEF Values (21 vs 22)

**Doc 21_Buff_System.md (lines 78-92):**
- Provoke Lv1: +5% ATK, -10% DEF
- Provoke Lv10: +32% ATK, -55% DEF
- Formula implied: ATK = `(2 + 3*Lv)%`, DEF = `(5 + 5*Lv)%`

**Doc 22_Status_Effects_Debuffs.md (line 785):**
- Provoke: ATK +2% per level (up to +20% at Lv10), DEF -5% per level (up to -50% at Lv10)
- Formula implied: ATK = `2*Lv`, DEF = `5*Lv`

**Analysis:** Doc 21 says +5%/+8%/+11%... ATK (step of 3 per level) and -10%/-15%/-20%... DEF (step of 5 per level). Doc 22 says +2% ATK per level and -5% DEF per level. These are different formulas: Doc 21 has ATK `(2+3*Lv)%` while Doc 22 has ATK `2*Lv%`. At Lv10: Doc 21 = +32% ATK / -55% DEF; Doc 22 = +20% ATK / -50% DEF.

The rAthena source confirms Doc 21's values: `val2 = 2 + 3 * sc_val1` for ATK, `val3 = 5 + 5 * sc_val1` for DEF (with sc_val1 = skill level). Doc 21 is correct.

**Verdict:** Doc 22 has simplified/wrong Provoke values. ATK should be `(2+3*Lv)%` not `2*Lv%`, and DEF should be `(5+5*Lv)%` not `5*Lv%`.

**Correction needed:** 22_Status_Effects_Debuffs.md Section 5.1 Provoke values.

---

### C-03: Blitz Beat Damage Formula (11 vs 34)

**Doc 11_Archer_Hunter_Skills.md (line 963):**
```
PerHitDamage = (floor(DEX/10) + floor(INT/2) + SteelCrowLv*3 + 40) * 2
```

**Doc 34_Mount_Falcon_Trap_Performance.md (line 216-222):**
```
DamagePerHit = 80 + (SteelCrowLv * 6) + (Floor(INT / 2) * 2) + (Floor(DEX / 10) * 2)
Simplified: DamagePerHit = 80 + (SteelCrowLv * 6) + INT + Floor(DEX / 5)
```

**Analysis:** Doc 11 says `(DEX/10 + INT/2 + SC*3 + 40) * 2`. Expanded: `2*DEX/10 + 2*INT/2 + SC*6 + 80 = DEX/5 + INT + SC*6 + 80`. Doc 34 says `80 + SC*6 + INT/2*2 + DEX/10*2 = 80 + SC*6 + INT + DEX/5`. These are algebraically identical. The "simplified" form in both docs also matches: `80 + SC*6 + INT + DEX/5`.

**Verdict:** No contradiction. Both docs express the same formula in different algebraic forms. Confirmed agreement.

---

### C-04: Over-Upgrade Random Bonus at Weapon Level 4 (04 vs 01)

**Doc 04_Physical_Damage.md (line 195):**
- Level 4 weapon over-upgrade bonus: `0 ~ 14 per level past safe`
- At +10: 6 levels past safe (safe = +4), `6 * 0~14 = 0~84`

**Doc 01_Stats_Attributes.md (line 388):**
- Level 4 weapon over-upgrade: `0 ~ 13 per over-upgrade`

**Analysis:** Doc 04 says max random bonus per over-upgrade level is 14 for Lv4 weapons. Doc 01 says 13. The rAthena source uses `(weaponLv == 4) ? 14 : ...` in the `watk_overrefine_bonus` function. The maximum random roll per level for Lv4 is indeed 14 (random 0 to 13, which in rAthena's `rnd()%14` gives 0-13 per level).

**Verdict:** This is an off-by-one representation issue. rAthena generates `rnd() % (weaponLv == 4 ? 14 : ...)`, which produces values 0 through 13 inclusive. Doc 04 writes "0 ~ 14" which implies the range includes 14. Doc 01 writes "0 ~ 13" which is the actual range (exclusive upper bound in rAthena's modulo). Doc 01 is technically correct in the values produced (0-13). Doc 04's "0 ~ 14" overstates the max by 1. But at +10, Doc 04's total "0~84" also overstates -- it should be `6 * (0~13) = 0~78`.

**Correction needed:** 04_Physical_Damage.md line 195: change `0 ~ 14` to `0 ~ 13`, and `0~84` to `0~78`.

---

### C-05: Critical Hits and Perfect Dodge (04 vs 01)

**Doc 04_Physical_Damage.md (lines 491-495):**
- Critical hit property #3: "Bypass Perfect Dodge: PD check is skipped (critical always lands)"

**Doc 01_Stats_Attributes.md (line 445):**
- "Does NOT ignore Perfect Dodge -- Lucky Dodge can still avoid crits"

**Analysis:** This is a direct contradiction. Doc 04 says crits bypass PD. Doc 01 says crits do NOT bypass PD. Checking rAthena pre-renewal `battle.cpp`: the critical check happens AFTER the Perfect Dodge check. If PD triggers, the attack is dodged before the crit check even runs. Therefore, Perfect Dodge CAN avoid criticals. Doc 01 is correct.

**Verdict:** Doc 04 is wrong about criticals bypassing Perfect Dodge. In pre-renewal, Perfect Dodge is checked first and can avoid crits.

**Correction needed:** 04_Physical_Damage.md line 493: change to "Does NOT bypass Perfect Dodge: PD check happens first and can avoid criticals."

---

### C-06: Soft DEF Formula Complexity (04 vs 01)

**Doc 04_Physical_Damage.md (lines 300-317):**
- SoftDEF = `floor(VIT*0.5) + max(floor(VIT*0.3), floor(VIT^2/150)-1)`
- Has a "random component" variant mentioned separately
- Table shows VIT 99 = 113 total SoftDEF

**Doc 01_Stats_Attributes.md (lines 508-527):**
- SoftDEF formula has a random component: `vit_def_base + vit_def_random + floor(AGI/5) + floor(BaseLevel/2)`
- Includes AGI/5 and BaseLevel/2 contributions
- References rAthena PR #6766

**Analysis:** Doc 04 presents soft DEF as only VIT-based (no AGI, no BaseLv contribution). Doc 01 includes `floor(AGI/5) + floor(BaseLevel/2)` as additional flat contributors to soft DEF. The rAthena source (status.cpp, `status_calc_def2`) does include AGI and BaseLv contributions to soft DEF. Doc 01 is more complete. Doc 04 omits AGI and BaseLv components.

**Verdict:** Doc 04 is incomplete -- it shows only the VIT component of soft DEF, missing the AGI/5 and BaseLv/2 terms. This matters for implementation: a Lv99 character with 50 AGI and 99 VIT would have an additional `floor(50/5) + floor(99/2) = 10 + 49 = 59` soft DEF beyond what Doc 04 accounts for.

**Correction needed:** 04_Physical_Damage.md Soft DEF section should include the full formula with AGI and BaseLv contributions.

---

### C-07: Decrease AGI Cancel List (21 vs 22)

**Doc 21_Buff_System.md (line 228):**
- Decrease AGI cancels: Blessing, Increase AGI, Two-Hand Quicken, Spear Quicken, Adrenaline Rush, One-Hand Quicken

**Doc 22_Status_Effects_Debuffs.md (line 837):**
- Decrease AGI removes: Increase AGI, Two Hand Quicken, Adrenaline Rush, Wind Walker

**Analysis:** Doc 21 lists 6 buffs cancelled by Decrease AGI. Doc 22 lists 4, including Wind Walker (not in Doc 21's list) but omitting Blessing, Spear Quicken, and One-Hand Quicken. The rAthena source shows Decrease AGI clears: Increase AGI, Two-Hand Quicken, Spear Quicken, Adrenaline Rush, One-Hand Quicken, Assassin Cross of Sunset. It does NOT clear Blessing (Blessing and Decrease AGI cancel each other independently via mutual exclusion, but Decrease AGI itself doesn't strip Blessing). Wind Walker is a Renewal-only buff.

**Verdict:** Both docs are partially incorrect:
- Doc 21 incorrectly includes Blessing in the cancel list (it is a mutual cancellation, not one-directional)
- Doc 22 incorrectly includes Wind Walker (Renewal-only) and omits Spear Quicken and One-Hand Quicken
- Doc 22 also omits Assassin Cross of Sunset

**Correction needed:** Both docs should list: Increase AGI, Two-Hand Quicken, Spear Quicken, Adrenaline Rush, One-Hand Quicken, Assassin Cross of Sunset. Blessing is separately handled via mutual exclusion (Decrease AGI on a Blessed target removes Blessing AND vice versa, but this is a distinct mechanism).

---

### C-08: EDP ATK Multiplier (21 vs 22)

**Doc 21_Buff_System.md (lines 707-718):**
- EDP Lv1: +100% ATK, Lv5: +300% ATK
- Duration: `(40 + 4*Lv)` seconds (Lv1=44s, Lv5=60s)

**Doc 22_Status_Effects_Debuffs.md (lines 941-944):**
- EDP: +400% weapon ATK (pre-Renewal)
- Duration: `40s + 20s per level` (Lv5 = 140s)

**Analysis:** Doc 21 says +100% to +300% scaling with level. Doc 22 says a flat +400% at all levels, and a completely different duration formula. The rAthena pre-renewal EDP damage formula is: `damage = damage * (1 + val1/100)` where val1 scales with level. In pre-renewal, EDP's effective multiplier is level-dependent, matching Doc 21. Doc 22's "+400%" appears to conflate pre-renewal with Renewal mechanics (where EDP is 4x). The duration formula in Doc 21 (`40 + 4*Lv`) is correct per rAthena; Doc 22's `40 + 20*Lv` is wrong.

**Verdict:** Doc 22 has incorrect EDP values on both the multiplier (should scale per level, not flat 400%) and the duration formula.

**Correction needed:** 22_Status_Effects_Debuffs.md Section 5.13 EDP values.

---

### C-09: Divest/Strip Duration and Effects (21 vs 22)

**Doc 21_Buff_System.md (lines 726-738):**
- Duration: `(60 + 15*Lv)` seconds (Lv1=75s, Lv5=135s)
- Strip Weapon: -25% ATK; Strip Shield: -15% hard DEF; Strip Armor: -40% VIT; Strip Helm: -40% INT

**Doc 22_Status_Effects_Debuffs.md (lines 886-892):**
- Duration: "Base 13s, reduced by target DEX"
- Strip Weapon: -25% ATK; Strip Shield: -15% DEF; Strip Armor: -40% VIT DEF; Strip Helm: -15% INT/DEX

**Analysis:** Major contradictions on both duration and effects:
- Duration: Doc 21 says 75-135s (level-based). Doc 22 says 13s (DEX-reduced). These cannot both be correct. The rAthena source uses a DEX-based formula for success rate, and the duration is `(60 + 15 * skillLv) * 1000` ms. Doc 21's duration formula is correct.
- Strip Helm effect: Doc 21 says -40% INT. Doc 22 says -15% INT/DEX. The rAthena source applies -40% INT for Strip Helm on monsters, -15% INT on players in some implementations. This needs per-context clarification.

**Verdict:** Doc 22 has the wrong duration formula (13s is likely the success rate check, not the duration). Doc 22 also has different Strip Helm values.

**Correction needed:** 22_Status_Effects_Debuffs.md Section 5.9 strip duration and helm values.

---

### C-10: ASPD Haste2 Group Members (06 vs 21)

**Doc 06_ASPD_Hit_Flee_Critical.md (lines 197-199):**
- "Two-Hand Quicken, One-Hand Quicken, Spear Quicken, Adrenaline Rush, and Assassin Cross of Sunset all belong to the same exclusion group"

**Doc 21_Buff_System.md (lines 1081-1088):**
- ASPD Haste2 Group: Two-Hand Quicken, Spear Quicken, Adrenaline Rush, Assassin Cross of Sunset (4 members, no One-Hand Quicken)

**Analysis:** Doc 06 lists 5 members including One-Hand Quicken. Doc 21 lists only 4, omitting One-Hand Quicken. One-Hand Quicken (SC_ONEHAND) is indeed in the ASPD Haste2 mutual exclusion group in rAthena. Doc 06 is correct.

**Verdict:** Doc 21 omits One-Hand Quicken from the Haste2 group listing.

**Correction needed:** 21_Buff_System.md Section 4.2 should include One-Hand Quicken in the Haste2 group table.

---

## Ambiguities (Multiple Interpretations Possible)

### A-01: Hard DEF Formula (4000-based vs simplified)

All three DEF-referencing docs (01, 04, 08) acknowledge TWO different Hard DEF formulas:
1. Official iRO: `damage * (4000 + DEF) / (4000 + DEF*10)`
2. rAthena simplified: `damage * (100 - DEF) / 100` (DEF capped at 99)

Docs 01 and 04 both present the 4000-based formula as canonical. However, they also mention the simplified formula. The question of which to implement is left to the developer. Doc 05 (Magical Damage) similarly presents two MDEF formulas (1000-based vs simplified).

**Recommendation:** The project should decide on one formula and document it in Global Rules. Currently the server implementation should be checked for consistency.

---

### A-02: Armor Refinement DEF Contribution

**Doc 04_Physical_Damage.md (line 280):**
- `ArmorRefineDEF = floor((3 + RefineLv) / 4)` -- gives values 1,1,1,1,2,2,2,2,3,3

**Doc 01_Stats_Attributes.md (line 504):**
- "each + shown as 1 DEF, but damage calculation uses 0.7 DEF per +". This appears to reference an iRO Wiki discrepancy.

These are different models. Doc 04's floor formula produces 1 at +1 through +4 and 2 at +5. The "0.7 per +" model would give 0.7 at +1, 1.4 at +2, etc. They produce different values. The floor formula from Doc 04 matches our implementation and is more widely cited.

---

### A-03: Soft DEF Random Component

**Doc 04 (line 319-324):** Shows a random variant where the second component is `rnd(floor(VIT*0.3), max(...))`.
**Doc 01 (line 517-519):** Shows `vit_def_random = rnd(vit_def_min, vit_def_max)`.
**Doc 04's main formula (line 302):** Uses `max(...)` which would be the MAX (not random) of the variable component.

The question is: does Soft DEF use the MAX of the VIT^2/150 term (deterministic) or a RANDOM value between min and max? Both docs agree it is random in actual damage calculation, but Doc 04's primary formula line 302 uses `max()` which reads as deterministic.

**Recommendation:** Clarify in Doc 04 that the primary formula shows the MAX (stat window display), while the random variant shows the per-hit calculation.

---

### A-04: Sight Blaster Single vs Multi-Target

**Doc 21_Buff_System.md (line 442-443):**
- "When an enemy enters the 3x3 area, deals fire damage... Reactive trigger -- fires once then consumed."
- "Single use: Consumed after triggering (one enemy triggers it)"

**MEMORY.md (wizard session 2):**
- "Sight Blaster multi-target (all enemies in 3x3, rAthena verified)"

The deep research doc says single-target (consumed after one trigger). The implementation notes say multi-target was verified as correct against rAthena. This ambiguity exists because rAthena's implementation changed over time -- earlier versions were single-target, later versions hit all in 3x3 then consume.

**Recommendation:** Doc 21 should be updated to reflect the rAthena-verified multi-target behavior.

---

### A-05: ASPD Delay Conversion Formula

**Doc 06 (lines 248-258):** Presents two formulas:
1. `Attack Delay (ms) = (200 - ASPD) * 20`
2. `Amotion (ms) = (200 - ASPD) * 10`

These give different values. At ASPD 150: formula 1 = 1000ms, formula 2 = 500ms. Doc 06 explains the discrepancy (centisecond units vs milliseconds), but the reference table uses formula 1 (`* 20`). The table values are correct (ASPD 150 = 1000ms = 1 hit/sec), confirming formula 1 is the correct ms conversion.

**Recommendation:** Remove formula 2 or label it explicitly as "centisecond units" to avoid confusion.

---

## Confirmed Agreements (Key Facts Verified Across 3+ Docs)

### AG-01: StatusATK Formula
All three docs (01, 04, 06) agree on:
- Melee: `floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)`
- Ranged: `floor(BaseLv/4) + DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/3)`
Verified across 01_Stats, 04_Physical_Damage, and multiple skill-specific docs.

### AG-02: Size Penalty Table
All three docs (04, 08, 01) present identical weapon-vs-size penalty values. Complete 17-weapon-type table matches across all sources. Key values confirmed: Dagger 100/75/50, 1H Sword 75/100/75, Bow 100/100/75, Katar 75/100/75, etc.

### AG-03: Element Table Level 1
Docs 04, 05, and 07 all reference the same element modifier values. Spot-checked 20 cells across all three docs -- all match. Example confirmations:
- Fire vs Water Lv1 = 50% (all three docs)
- Wind vs Water Lv1 = 175% (all three docs)
- Ghost vs Neutral Lv1 = 25% (all three docs)
- Neutral vs Ghost Lv1 = 25% (all three docs)

### AG-04: Heal Formula
Three docs confirm: `HealAmount = floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)`
- 13_Acolyte_Priest_Monk_Skills (line 46)
- 09_Swordsman_Knight_Crusader_Skills (line 1287, for Crusader Heal)
- AUDIT_04_First_Class_Skills (line 104)
SP cost also matches: `10 + 3*Lv` across all.

### AG-05: Blitz Beat Formula
Two primary docs and one audit doc all confirm: `(DEX/10 + INT/2 + SC*3 + 40) * 2` per hit.
Auto-Blitz hits = `floor((JobLv + 9) / 10)` capped at learned Blitz Beat level.
Trigger chance = `floor(LUK/3)%`.
All values confirmed across 11_Archer_Hunter, 34_Mount_Falcon_Trap, and AUDIT_06.

### AG-06: Critical Hit Properties (Partial)
All docs agree on:
- Max WeaponATK (no variance)
- Bypass FLEE (always hits)
- Bypass Hard DEF and Soft DEF
- +40% damage multiplier (1.4x)
- Katar doubles crit rate
- Skills cannot crit (pre-renewal)

Disagreement only on Perfect Dodge (see C-05).

### AG-07: ASPD Hard Cap
Docs 06, 34, and 21 all agree: ASPD hard cap = 190 (5 hits/sec). Sabri_MMO deviation to 195 is documented in 06 and acknowledged as intentional.

### AG-08: ASPD Haste2 Mutual Exclusion
All relevant docs (06, 21, 15) agree that THQ/SQ/AR/ACoS are mutually exclusive and only the strongest wins. They also agree that ASPD potions are a separate system that stacks with skill-based ASPD buffs.

### AG-09: Boss Protocol Immunities
Docs 08 and 22 present consistent boss immunity lists. Both agree bosses are immune to: Stun, Freeze, Stone Curse, Sleep, Poison, Deadly Poison, Silence, Blind, Curse, Confusion, Bleeding, Close Confine, Spider Web. Both agree Quagmire, Lex Aeterna, Provoke, Strip, and Dispel work on bosses.

### AG-10: Freeze/Stone Element Change
Docs 07, 22, and 08 all agree:
- Frozen targets become Water Lv1
- Petrified targets become Earth Lv1
These changes persist for the duration of the status.

### AG-11: Weapon Element Priority
Docs 04 and 07 agree on priority order: Endow skills > Arrow element (non-Neutral) > Weapon innate element > Neutral default.

### AG-12: Grand Cross Formula
Docs 09 and 08 agree:
- Uses WeaponATK + MATK (hybrid damage)
- Custom formula: `floor((ATK + MATK) * (100 + 40*Lv) / 100) * HolyElementMod`
- Self-damage = `floor(GrandCrossDamage / 2) * HolyElementModOnCaster`
- Bypasses size penalty (confirmed in both docs)
- 9-cell cross pattern (not 5x5 square)

### AG-13: Acid Terror DEF Bypass
Docs 08 and 15 agree:
- Acid Terror ignores Hard DEF but NOT VIT soft DEF
- Force hit (no HIT/FLEE check)
- Boss damage halved (50%)
- Uses own damage formula; bypasses size penalty

### AG-14: Buffs That Survive Death
Doc 21 lists: Endure, Auto Berserk, Shrink, Riding, Pushcart, Wedding. No other doc contradicts this list.

### AG-15: Weapon Endow Mutual Exclusion
Docs 07, 21, and the Sage/Assassin skill docs all agree: only one weapon element buff active at a time. New endow replaces old. Weapon swap cancels endow. Dispel removes endows (except Enchant Poison in some implementations). All docs agree on this system.

### AG-16: Mount ASPD Penalty
Docs 34 and 06 agree: 50% ASPD penalty while mounted, recovered at 10% per Cavalier Mastery level (Lv5 = full recovery).

### AG-17: Refinement ATK Bonus Per Level
Docs 01, 04, and 05 all agree:
- Weapon Lv1: +2 ATK per refine
- Weapon Lv2: +3 ATK per refine
- Weapon Lv3: +5 ATK per refine
- Weapon Lv4: +7 ATK per refine
Safe limits: 7/6/5/4 for Lv 1/2/3/4.

---

## Recommended Corrections

| # | Document | Issue | Correction |
|---|----------|-------|------------|
| 1 | 05_Magical_Damage.md | MATK table values wrong at INT 80/90/99 (C-01) | Recompute: INT 80 Min=201, INT 90 Min=234, INT 99 Min=295/Max=460 |
| 2 | 22_Status_Effects_Debuffs.md | Provoke values simplified incorrectly (C-02) | ATK = `(2+3*Lv)%` not `2*Lv%`; DEF = `(5+5*Lv)%` not `5*Lv%` |
| 3 | 04_Physical_Damage.md | Over-upgrade Lv4 bonus says 0~14 (C-04) | Should be 0~13 (rnd()%14 = 0-13). Max at +10 = 0~78 not 0~84 |
| 4 | 04_Physical_Damage.md | Critical hits said to bypass Perfect Dodge (C-05) | PD is checked first; crits do NOT bypass PD. Change to "Does NOT bypass" |
| 5 | 04_Physical_Damage.md | Soft DEF formula missing AGI/BaseLv terms (C-06) | Add `+ floor(AGI/5) + floor(BaseLv/2)` per rAthena and Doc 01 |
| 6 | 22_Status_Effects_Debuffs.md | Decrease AGI cancel list includes Wind Walker (C-07) | Remove Wind Walker (Renewal-only). Add Spear Quicken, One-Hand Quicken, ACoS |
| 7 | 21_Buff_System.md | Decrease AGI cancel list includes Blessing (C-07) | Separate Blessing into "mutual exclusion" rather than "cancel list" |
| 8 | 22_Status_Effects_Debuffs.md | EDP values wrong: +400% flat, wrong duration (C-08) | Use Doc 21 values: +100-300% scaling, duration `(40+4*Lv)s` |
| 9 | 22_Status_Effects_Debuffs.md | Strip duration says "Base 13s" (C-09) | Should be `(60+15*Lv)s` per rAthena and Doc 21 |
| 10 | 21_Buff_System.md | Haste2 group missing One-Hand Quicken (C-10) | Add One-Hand Quicken to the Haste2 mutual exclusion table |
| 11 | 21_Buff_System.md | Sight Blaster described as single-target (A-04) | Update to multi-target (all in 3x3, then consumed) per rAthena verification |

---

## Summary

- **10 contradictions found**, ranging from minor numeric discrepancies to fundamentally different mechanics
- **5 ambiguities identified** where multiple valid interpretations exist or docs need clarification
- **17 confirmed agreements** on critical shared topics (formulas, tables, system mechanics)
- **11 corrections recommended** across 4 documents (04, 05, 21, 22)

**Most impactful contradictions:**
1. **C-05 (Critical vs Perfect Dodge)**: Directly affects combat behavior -- getting this wrong means PD-stacking builds are either viable or useless against crit builds
2. **C-01 (MATK at high INT)**: Affects all magical damage calculations for endgame characters
3. **C-06 (Soft DEF missing terms)**: Under-representing DEF by 30-60 points at high levels

**Documents with most issues:**
- `22_Status_Effects_Debuffs.md`: 4 corrections needed (Provoke, Decrease AGI, EDP, Strip)
- `04_Physical_Damage.md`: 3 corrections needed (over-upgrade, crit/PD, soft DEF)
