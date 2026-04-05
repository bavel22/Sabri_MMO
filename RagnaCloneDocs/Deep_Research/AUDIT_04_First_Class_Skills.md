# AUDIT 04: First Class Skills — Deep Research vs Server Implementation

> **Auditor**: Claude Opus 4.6 (1M context)
> **Date**: 2026-03-22
> **Scope**: All 1st-class skills — Swordsman (100-109), Mage (200-213), Archer (300-306), Acolyte (400-414), Thief (500-509), Merchant (600-609)
> **Sources Compared**: Deep Research docs (09/10/11/13/14/15) vs `server/src/index.js` handlers + `server/src/ro_skill_data.js` data entries
> **Method**: For each skill — compare formula, SP cost, cast time, cooldown, ACD, special mechanics between research and implementation

---

## Summary

| Class | Skills | PASS | MINOR | ISSUE | MISSING |
|-------|--------|------|-------|-------|---------|
| Swordsman | 10 | 9 | 1 | 0 | 0 |
| Mage | 14 | 11 | 2 | 1 | 0 |
| Archer | 7 | 6 | 0 | 1 | 0 |
| Acolyte | 15 | 14 | 1 | 0 | 0 |
| Thief | 10 | 9 | 1 | 0 | 0 |
| Merchant | 10 | 10 | 0 | 0 | 0 |
| **TOTAL** | **66** | **59** | **5** | **2** | **0** |

**Legend**:
- **PASS**: Formula, SP, cast time, and special mechanics all match research
- **MINOR**: Cosmetic or negligible difference, functionally correct
- **ISSUE**: Formula or mechanic mismatch that affects gameplay
- **MISSING**: Skill handler or data entry does not exist

---

## 1. Swordsman Skills (IDs 100-109)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 100 | Sword Mastery | Passive (getPassiveSkillBonuses) | YES — `smLv * 4` ATK for dagger/1h_sword | N/A | N/A | Non-stacking with Rogue 1705 via `Math.max()` | PASS |
| 101 | Two-Handed Sword Mastery | Passive (getPassiveSkillBonuses) | YES — `tsmLv * 4` ATK for 2h_sword | N/A | N/A | Correct weapon filter | PASS |
| 102 | Increase HP Recovery | Passive (getPassiveSkillBonuses) | YES — `hprLv * 5` HP/tick | N/A | N/A | Applied in regen tick | PASS |
| 103 | Bash | skill:use handler (line ~9599) | YES — `130+i*30` = 130-400% ATK | YES — Lv1-5: 8, Lv6-10: 15 | YES — 0ms | YES — Fatal Blow stun `(lv-5)*baseLv/10`, HIT bonus `5*lv%`, Safety Wall block, Energy Coat check | PASS |
| 104 | Provoke | skill:use handler (line ~9761) | YES — ATK+: `2+3*lv`%, DEF-: `5+5*lv`% | YES — `4+i` = 4-13 | YES — 0ms, 1s CD | YES — Boss/Undead immune (pre-SP), success `50+3*lv`%, aggro force, Play Dead break | PASS |
| 105 | Magnum Break | skill:use handler (line ~9867) | YES — `120+i*20` = 120-300% Fire ATK | YES — 30 flat | YES — 0ms cast, 2s ACD | YES — HP cost `21-ceil(lv/2)`, can't kill, 2-cell knockback, fire endow buff, HIT `+10*lv`, 5x5 AoE, zone filter | PASS |
| 106 | Endure | skill:use handler (line ~10067) | YES — MDEF `+lv`, duration `10+3*lv`s | YES — 10 flat | YES — 0ms, 10s CD | YES — 7-hit counter, in BUFFS_SURVIVE_DEATH | PASS |
| 107 | Moving HP Recovery | Passive (getPassiveSkillBonuses) | YES — sets `movingHPRecovery = true` | N/A | N/A | Passive flag used in regen tick | PASS |
| 108 | Auto Berserk | skill:use handler (line ~13061) | YES — toggle, Provoke Lv10 effect (+32% ATK) | YES — 1 SP | YES — 0ms | YES — Toggle on/off, HP<25% trigger, survives death | PASS |
| 109 | Fatal Blow | Passive (getPassiveSkillBonuses) | YES — `fbLv * 5` passive flag | N/A | N/A | MINOR — fatalBlowChance set to `fbLv*5` (=5 for Lv1) but handler uses `(bashLv-5)*baseLv/10` correctly. The passive flag is only a boolean check (`> 0`), not used as the actual stun chance. Functionally correct. | PASS |

**Swordsman Notes**:
- All 10 skills fully implemented and matching research
- Provoke checks Boss/Undead immunity BEFORE SP deduction (correct per research)
- Magnum Break correctly self-centered (not ground-targeted)
- MINOR on 109: The `fatalBlowChance` value of `fbLv*5` in the passive bonuses is never used as a percentage; the handler derives stun chance from `(bashLevel-5)*baseLevel/10` which is the correct rAthena formula. No gameplay impact.

---

## 2. Mage Skills (IDs 200-213)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 200 | Cold Bolt | Bolt handler (line ~10114) | YES — 100% MATK * hits (hits=level) | YES — `12+i*2` = 12-30 | YES — `700*(i+1)` = 700-7000ms | YES — Magic Rod check, Lex Aeterna on bundle, per-hit staggered damage numbers | PASS |
| 201 | Fire Bolt | Same bolt handler | YES — same formula, Fire element | YES — same | YES — same | Same mechanics, correct element | PASS |
| 202 | Lightning Bolt | Same bolt handler | YES — same formula, Wind element | YES — same | YES — same | Same mechanics, correct element | PASS |
| 203 | Napalm Beat | Handler (line ~10423) | YES — `80+i*10` = 80-170% MATK Ghost, damage split | YES — `[9,9,9,12,12,12,15,15,15,18]` | YES — 1000ms fixed | YES — 3x3 splash, damage split among targets, zone filter | PASS |
| 204 | Increase SP Recovery | Passive (getPassiveSkillBonuses) | YES — `sprLv * 3` SP/tick | N/A | N/A | Applied in regen tick | PASS |
| 205 | Sight | Handler (line ~11155) | YES — reveal hidden, 10s duration | YES — 10 SP | YES — 0ms | YES — 7x7 AoE reveal, self buff | PASS |
| 206 | Stone Curse | Handler (line ~11047) | YES — `24+i*4` = 24-60% chance | YES — `25-i` = 25-16 | YES — 1000ms fixed | YES — Two-phase petrification, Boss/Undead immune | PASS |
| 207 | Fire Ball | Handler (line ~10573) | YES — `80+i*10` = 80-170% MATK Fire | YES — 25 flat | YES — Lv1-5: 1500, Lv6-10: 1000ms | YES — 5x5 splash, full damage (not split) | PASS |
| 208 | Frost Diver | Handler (line ~10907) | YES — `110+i*10` = 110-200% MATK Water | YES — `25-i` = 25-16 | YES — 800ms, 1500ms ACD | YES — Freeze chance `35+3*lv`%, freeze duration `3*lv`s, Boss/Undead immune | PASS |
| 209 | Fire Wall | Handler (line ~11184) | YES — 50% MATK per hit, hits=`lv+2` | YES — 40 flat | YES — `2150-150*lv`ms = 2000-650ms | YES — Ground effect, max 3, knockback 2, Undead pass through | PASS |
| 210 | Soul Strike | Handler (line ~10282) | YES — 100% MATK * hits, hits=`floor((lv+1)/2)` | YES — zigzag `[18,14,24,20,30,26,36,32,42,38]` | YES — 500ms fixed | YES — Ghost element, Undead bonus `+5%*lv`, Magic Rod check, Lex Aeterna | PASS |
| 211 | Safety Wall | Handler (line ~11254) | YES — hits blocked=`lv+1`=2-11 | MINOR — data `effectValue: i+2` = 2-11 hits. Research says `lv+1` = 2-11. Same result. | YES — `[4000,3500,...,1000]` | YES — 1x1 cell, Blue Gemstone, overlap prevention with Pneuma | PASS |
| 212 | Thunderstorm | Handler (line ~10705) | YES — 80% MATK per hit, hits=level | YES — `24+(i+1)*5` = 29-74 | YES — `(i+1)*1000` = 1-10s | YES — 5x5 AoE ground, Wind element, zone filter | PASS |
| 213 | Energy Coat | Handler (line ~13084) | YES — buff applied with 300s duration | YES — 30 SP | ISSUE — Data has `castTime: 5000`. Research says `castTime: 0` (instant). Energy Coat is a quest skill with no cast time in RO Classic. | YES — SP-based damage reduction tiers in `applyEnergyCoat()` | ISSUE |

**Mage Notes**:
- **ISSUE**: Energy Coat (213) has `castTime: 5000` in `ro_skill_data.js` but research confirms it should be `0` (instant cast). This is a quest skill that activates immediately in RO Classic.
- MINOR on 211: `effectValue` uses `i+2` which equals `level+1` since `i` is zero-indexed. Functionally identical.
- All bolt spells share a clean handler with correct per-hit staggered damage display.

---

## 3. Archer Skills (IDs 300-306)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 300 | Owl's Eye | Passive (getPassiveSkillBonuses) | YES — `+oeLv` DEX | N/A | N/A | Flat +1-10 DEX | PASS |
| 301 | Vulture's Eye | Passive (getPassiveSkillBonuses) | YES — `+veLv` HIT, `+veLv*50` range (bow) | N/A | N/A | Non-stacking with Rogue 1706 via `Math.max()`, range is bow-only | PASS |
| 302 | Improve Concentration | Handler (line ~12303) | YES — `(2+lv)%` AGI/DEX buff | YES — `25+i*5` = 25-70 | YES — 0ms | YES — Self buff, duration `40+20*lv`s, reveal hidden on cast | PASS |
| 303 | Double Strafe | Handler (line ~12182) | ISSUE — Data has `effectValue: 100+i*10` = 100-190% per hit, handler uses `effectVal * 2` = 200-380% total. Research says per-hit `(90+10*Lv)%` = 100-190%, total = 200-380%. Data stores per-hit as `100+i*10` which for Lv1 gives 100% per hit (total 200%), but research says Lv1 per-hit is `90+10*1=100%`. Actually matches! | YES — 12 flat | YES — 0ms cast, 0ms ACD | YES — Bundled single hit, bow required, 1 arrow consumed, Vulture's Eye range | PASS |
| 304 | Arrow Shower | Handler (line ~12244) | YES — `80+i*5` = 80-125% ATK | YES — 15 flat | YES — 0ms cast, 1000ms ACD | YES — 5x5 AoE ground, knockback 2 from center, bow/arrow required, zone filter | PASS |
| 305 | Arrow Crafting | Handler (line ~20309) | YES — opens crafting UI | YES — 10 SP | YES — 0ms | YES — weight check <50%, recipe system in `ro_arrow_crafting.js` | PASS |
| 306 | Arrow Repel | Handler (line ~12358) | YES — 150% ATK, 6-cell knockback | YES — 15 SP | ISSUE — Data has `castTime: 1500`. Research says 1500ms but notes conflicting sources (iRO: not interruptible, rAthena: interruptible). The 1500ms matches. However, research notes potential issue with interruptibility. | YES — Bow required, arrow consumed, knockback 6 from caster, Pneuma blocks | PASS |

**Archer Notes**:
- All 7 skills fully implemented
- Double Strafe correctly implements bundled damage (single `calculateSkillDamage` call at `totalMultiplier`)
- Arrow Shower correctly uses ground target position for knockback direction (not caster position)
- ISSUE on 303 resolved on closer inspection: the per-hit formula `100+i*10` with i=0 gives 100% at Lv1, and research says `90+10*1=100%`, so they match.

---

## 4. Acolyte Skills (IDs 400-414)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 400 | Heal | Handler (line ~11318) | YES — `floor((baseLv+INT)/8) * (4+8*skillLv)` | YES — `13+i*3` = 13-40 | YES — 0ms cast, 1000ms ACD | YES — Undead damage (holy, halved), healPower bonuses, friendly player targeting | PASS |
| 401 | Divine Protection | Passive (getPassiveSkillBonuses) | YES — rAthena formula `floor((baseLv/25+3)*lv+0.5)` DEF vs Undead/Demon | N/A | N/A | Applied to raceDEF.undead and raceDEF.demon | PASS |
| 402 | Blessing | Handler (line ~11416) | YES — `+lv` STR/DEX/INT | YES — `28+i*4` = 28-64 | YES — 0ms | YES — Cure Curse/Stone, debuff Undead/Demon (halve stats), duration `40+20*lv`s | PASS |
| 403 | Increase AGI | Handler (line ~11526) | YES — `+3+i` = 3-12 AGI, +25% movespeed | YES — `18+i*3` = 18-45 | YES — 1000ms cast, 1000ms ACD | YES — 15 HP cost, min HP 16, cancels Decrease AGI | PASS |
| 404 | Decrease AGI | Handler (line ~11579) | YES — `-3-i` = 3-12 AGI, -25% movespeed | YES — `15+i*2` = 15-33 | YES — 1000ms cast, 1000ms ACD | YES — Success rate `40+2*lv+floor((baseLv+INT)/5)-MDEF`, Boss immune, strips ASPD buffs, SP consumed on fail, separate monster/player durations | PASS |
| 405 | Cure | Handler (line ~11675) | YES — removes Silence/Blind/Confusion | YES — 15 SP | YES — 0ms | YES — Confusion on Undead targets | PASS |
| 406 | Angelus | Handler (line ~11739) | YES — `+5+i*5` = 5-50% VIT DEF | YES — `23+i*3` = 23-50 | YES — 500ms cast, 3500ms ACD | YES — Party-wide, duration `(i+1)*30`s | PASS |
| 407 | Signum Crucis | Handler (line ~11750) | YES — `14+i*4` = 14-50% DEF reduction on Undead/Demon | YES — 35 flat | YES — 500ms cast, 2000ms ACD | YES — Success rate `23+4*lv+casterLv-targetLv`, permanent duration, screen-wide AoE | PASS |
| 408 | Ruwach | Handler (line ~11780) | YES — reveals hidden, 145% MATK Holy damage | YES — 10 SP | YES — 0ms | YES — 5x5 AoE, 10s duration | PASS |
| 409 | Teleport | Handler (line ~11804) | YES — Lv1 random, Lv2 save point | YES — Lv1: 10, Lv2: 9 | YES — 0ms | YES — Cross-zone support | PASS |
| 410 | Warp Portal | Handler (line ~11922) | YES — ground portal with destination | YES — Lv1-4: 35,32,29,26 | YES — 1000ms | YES — Blue Gemstone, memo system (/memo1-3), max 3 active portals, duration `5+5*lv`s | PASS |
| 411 | Pneuma | Handler (line ~12054) | YES — blocks ranged physical | YES — 10 SP | YES — 0ms | YES — 3x3 ground, 10s duration, overlap prevention | PASS |
| 412 | Aqua Benedicta | Handler (line ~12086) | YES — creates Holy Water | YES — 10 SP | YES — 1000ms cast, 500ms ACD | MINOR — Simplified implementation (no water cell check). Research says must stand on water cell or Deluge. | PASS |
| 413 | Demon Bane | Passive (getPassiveSkillBonuses) | YES — rAthena formula `floor(lv*(baseLv/20+3))` ATK vs Undead/Demon | N/A | N/A | Applied to raceATK.undead and raceATK.demon | PASS |
| 414 | Holy Light | Handler (line ~12096) | YES — 125% MATK Holy | YES — 15 SP | YES — 2000ms | YES — Quest skill, Kyrie Eleison cancel on target | PASS |

**Acolyte Notes**:
- All 15 skills fully implemented
- Heal formula exactly matches rAthena: `floor((baseLv+INT)/8) * (4+8*skillLv)` in `calculateHealAmount()`
- Decrease AGI correctly strips 6 ASPD buffs (adrenaline_rush, adrenaline_rush_full, two_hand_quicken, one_hand_quicken, spear_quicken, cart_boost) — matches research
- MINOR on 412: Aqua Benedicta does not check for water cell requirement (simplified). Not a formula issue.
- Warp Portal memo system fully implemented with /memo1-3 chat commands and destination popup

---

## 5. Thief Skills (IDs 500-509)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 500 | Double Attack | Passive (getPassiveSkillBonuses) | YES — `daLv * 5`% chance, dagger-only | N/A | N/A | Proc check in combat tick for auto-attacks only | PASS |
| 501 | Improve Dodge | Passive (getPassiveSkillBonuses) | MINOR — `idLv * 3` FLEE. Research says 1st class=+3/lv, 2nd class=+4/lv. Implementation uses flat +3/lv for all classes. Missing 2nd class scaling. | N/A | N/A | No Perfect Dodge bonus (correct) | MINOR |
| 502 | Steal | Handler (line ~12422) | YES — success `(4+6*lv)+(DEX-monDEX)/2` | YES — 10 SP flat | YES — 0ms | YES — Boss immune, one-steal-per-monster, item selection from drop table | PASS |
| 503 | Hiding | Handler (line ~12511) | YES — toggle, duration `30*lv`s | YES — 10 SP activation | YES — 0ms | YES — SP drain `1 SP per (4+lv)s`, stops auto-attack, Close Confine break, hidden buff | PASS |
| 504 | Envenom | Handler (line ~12600) | YES — 100% ATK Poison + flat `15*lv` bypass DEF | YES — 12 SP flat | YES — 0ms | YES — Flat bonus always hits even on miss, Poison chance `(10+4*lv)%`, Boss/Undead immune to poison, 60s poison duration, element modifier on flat bonus | PASS |
| 505 | Detoxify | Handler (line ~12703) | YES — removes Poison status | YES — 10 SP | YES — 0ms | YES — Can target self or other players | PASS |
| 506 | Sand Attack | Handler (line ~12729) | YES — 130% ATK Earth | YES — 9 SP | YES — 0ms | YES — 20% blind chance, uses `executePhysicalSkillOnEnemy` helper | PASS |
| 507 | Back Slide | Handler (line ~12744) | YES — 5 cells backward | YES — 7 SP | YES — 0ms | YES — Server position update, direction-based, blocks at obstacles | PASS |
| 508 | Throw Stone | Handler (line ~12799) | YES — fixed 50 damage | YES — 2 SP | YES — 0ms | YES — Always hits (forceHit), 3% stun, consumes Stone (7049), DEF reduces | PASS |
| 509 | Pick Stone | Handler (line ~12890) | YES — adds Stone (7049) to inventory | YES — 3 SP | YES — 500ms cast | YES — Weight check <50% | PASS |

**Thief Notes**:
- All 10 skills fully implemented
- **MINOR on 501**: Improve Dodge always gives +3 FLEE/level regardless of class. Research says 2nd class (Assassin/Rogue) should get +4/level. This slightly underpowers Assassin/Rogue FLEE at higher levels (max 30 FLEE instead of 40 at Lv10). Low priority but technically inaccurate.
- Envenom correctly handles the unique hybrid damage model: main ATK can miss but flat bonus always lands
- Hiding correctly implements all break conditions including Close Confine break

---

## 6. Merchant Skills (IDs 600-609)

| Skill ID | Name | Handler | Formula Match | SP Match | Cast Match | Special Mechanics | Status |
|----------|------|---------|---------------|----------|------------|-------------------|--------|
| 600 | Enlarge Weight Limit | Passive (getPlayerMaxWeight) | YES — `+200*lv` weight | N/A | N/A | Applied in `getPlayerMaxWeight()` on top of `2000+STR*30` base | PASS |
| 601 | Discount | Passive (getDiscountPercent) | YES — `[7,9,11,13,15,17,19,21,23,24]%` | N/A | N/A | Non-stacking with Rogue Compulsion Discount via `Math.max()`, min price=1 | PASS |
| 602 | Overcharge | Passive (getOverchargePercent) | YES — `[7,9,11,13,15,17,19,21,23,24]%` | N/A | N/A | Applied in shop:data handler | PASS |
| 603 | Mammonite | Handler (line ~12918) | YES — `150+i*50` = 150-600% ATK | YES — 5 SP flat | YES — 0ms cast, 0ms ACD | YES — Zeny cost `100*lv`, Dubious Salesmanship -10%, zeny checked/deducted before damage, Safety Wall block, uses `executePhysicalSkillOnEnemy` | PASS |
| 604 | Pushcart | Passive (cart system) | YES — enables cart rental | N/A | N/A | Speed penalty `(50+5*lv)%` applied in movement system, cart 8000 max weight | PASS |
| 605 | Vending | Handler (line ~20295) | YES — opens vending setup | YES — 30 SP | YES — 0ms | YES — Cart required, slots `2+lv` = 3-12 | PASS |
| 606 | Item Appraisal | Handler (line ~20250) | YES — identifies one item | YES — 10 SP | YES — 0ms | YES — Opens identify UI, one item per cast | PASS |
| 607 | Change Cart | Handler (line ~20280) | YES — changes cart appearance | YES — 40 SP | YES — 0ms | YES — Cart required, appearance by base level | PASS |
| 608 | Cart Revolution | Handler (line ~12946) | YES — `150 + floor(100*cartWeight/8000)%` = 150-250% | YES — 12 SP | YES — 0ms | YES — Cart required, forced Neutral element, forceHit, 3x3 splash, knockback 2, Lex Aeterna per-target | PASS |
| 609 | Loud Exclamation | Handler (line ~13043) | YES — +4 STR self buff, 300s | YES — 8 SP | YES — 0ms | YES — Self only (pre-renewal), cancelled by Quagmire | PASS |

**Merchant Notes**:
- All 10 skills fully implemented with zero issues
- Cart Revolution correctly implements forced Neutral element (cannot bypass Ghost property)
- Mammonite correctly checks zeny before deducting and includes Dubious Salesmanship quest skill interaction
- Discount/Overcharge both use exact per-level arrays `[7,9,11,13,15,17,19,21,23,24]` matching research

---

## Issues Summary

### ISSUE (2 total — affects gameplay)

| # | Skill | ID | Issue | Severity | Fix |
|---|-------|----|-------|----------|-----|
| 1 | Energy Coat | 213 | `castTime: 5000` in `ro_skill_data.js`. Should be `0` (instant). Energy Coat is a quest skill that activates immediately in RO Classic. A 5-second cast time makes it nearly unusable in combat. | Medium | Change `castTime: 5000` to `castTime: 0` in skill data line 48 |
| 2 | Improve Dodge | 501 | Missing 2nd class scaling. Always `+3 FLEE/lv`. 2nd class (Assassin/Rogue) should get `+4 FLEE/lv`. Max FLEE diff: 30 vs 40 at Lv10. | Low | In `getPassiveSkillBonuses()`, check player class: `const is2nd = ['assassin','rogue'].includes(player.jobClass?.toLowerCase()); bonuses.bonusFLEE += idLv * (is2nd ? 4 : 3);` |

### MINOR (5 total — no gameplay impact)

| # | Skill | ID | Note |
|---|-------|----|------|
| 1 | Fatal Blow | 109 | `fatalBlowChance = fbLv * 5` in passive bonuses is never used as a percentage; actual stun logic in Bash handler uses the correct `(bashLv-5)*baseLv/10` formula. Harmless dead value. |
| 2 | Safety Wall | 211 | `effectValue: i+2` uses zero-indexed `i` to produce 2-11. Same result as `lv+1` = 2-11. Correct but confusing. |
| 3 | Energy Coat | 213 | Data `effectValue: 30` is stored but the actual damage reduction uses the SP-tier table in `applyEnergyCoat()`. The 30 value is unused. |
| 4 | Aqua Benedicta | 412 | Missing water cell requirement (simplified). Player can create Holy Water anywhere. Low priority cosmetic restriction. |
| 5 | Arrow Repel | 306 | Cast interruptibility unclear in research (iRO vs rAthena conflict). Implementation uses standard cast interruption rules. Acceptable. |

---

## Verification Methodology

For each skill, the following was checked:

1. **Data entry** in `ro_skill_data.js`: skill ID, name, maxLevel, type, targetType, element, range, SP cost formula, cast time formula, ACD, cooldown, effectValue (damage %), duration, prerequisites
2. **Handler** in `index.js`: skill name match, damage calculation call, SP deduction, delay application, special mechanic implementation
3. **Passive implementation** in `getPassiveSkillBonuses()` or `getEffectiveStats()`: bonus type, formula, weapon/class restrictions
4. **Cross-reference** with deep research doc: formula comparison, SP cost per-level verification, cast time per-level verification, special mechanic presence

### Files Examined
- `server/src/ro_skill_data.js` — lines 23-98 (all 1st-class skill data entries)
- `server/src/index.js` — skill handlers at lines ~9599-13092 (combat skills), ~20248-20320 (utility skills), ~641-760 (passive bonuses), ~3781-3832 (getEffectiveStats), ~4684-4749 (economy helpers)
- `RagnaCloneDocs/Deep_Research/09_Swordsman_Knight_Crusader_Skills.md` — Swordsman section
- `RagnaCloneDocs/Deep_Research/10_Mage_Wizard_Sage_Skills.md` — Mage section
- `RagnaCloneDocs/Deep_Research/11_Archer_Hunter_Skills.md` — Archer section
- `RagnaCloneDocs/Deep_Research/13_Acolyte_Priest_Monk_Skills.md` — Acolyte section
- `RagnaCloneDocs/Deep_Research/14_Thief_Assassin_Rogue_Skills.md` — Thief section
- `RagnaCloneDocs/Deep_Research/15_Merchant_Blacksmith_Alchemist_Skills.md` — Merchant section
