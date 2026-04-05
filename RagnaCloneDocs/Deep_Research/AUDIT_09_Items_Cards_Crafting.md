# Audit: Items, Cards, Refining & Crafting

**Audit Date**: 2026-03-22
**Deep Research Sources**: `18_Items_Equipment.md`, `19_Card_System.md`, `20_Refining_Crafting.md`
**Server Files Audited**: `index.js` (equip/unequip/refine/forge/pharmacy/identify/compound handlers), `ro_card_effects.js`, `ro_item_effects.js`, `ro_arrow_crafting.js`, `ro_damage_formulas.js`

---

## Summary

The implementation covers the core systems well. The card system is particularly strong at 538 cards with 65+ bonus types, full damage pipeline integration, and a working compound UI. Refining, forging, and arrow crafting are functional. However, several discrepancies exist between the deep research and the actual implementation, ranging from a formula bug in forging to missing recipe sets.

**Overall Coverage**:
- Equipment System: ~90% (strong fundamentals, some missing validations)
- Card System: ~92% (excellent coverage, missing combos and skill-granting cards)
- Refining System: ~95% (nearly complete, minor missing features)
- Forging System: ~70% (formula bug + very few recipes)
- Pharmacy System: ~75% (missing condensed potions and some recipes)
- Arrow Crafting: ~98% (complete)

---

## Equipment System (slots, restrictions, weapon types)

### Correctly Implemented
- **10 equipment positions** with proper slot tracking (weapon, shield, armor, garment, footgear, head_top, head_mid, head_low, accessory_1, accessory_2, ammo)
- **Two-handed weapon rule**: shield auto-unequip on 2H equip, shield equip blocked with 2H weapon (lines 22960-22975)
- **Dual wield** (Assassin only): left-hand weapon support, katar mutual exclusion, per-hand damage percentages (50+10*RHM%, 30+10*LHM%)
- **Job/class restrictions**: `canJobEquip()` check on equip (line 22777)
- **Gender restrictions**: `canGenderEquip()` check (line 22783)
- **Level restrictions**: both `required_level` min (line 22771) and `equip_level_max` cap (line 22789)
- **Equipment stat bonuses**: full stat tracking (STR/AGI/VIT/INT/DEX/LUK, MaxHP/MaxSP, HIT/Flee/CRIT/PD, DEF/MDEF, rate modifiers)
- **Unidentified item blocking**: cannot equip unidentified items (line 22766)
- **Ammunition system**: ammo equip slot, element priority (Endow > Arrow > Weapon), per-attack consumption, status arrow procs
- **Size penalty table**: complete 17-weapon-type table in `ro_damage_formulas.js` matching deep research exactly

### Discrepancies
| Feature | Deep Research | Implementation | Severity |
|---------|-------------|----------------|----------|
| Headgear combo positions | Bitmask system (256/512/1 for Upper/Mid/Lower) with multi-slot items | Not verified -- headgear multi-position combos may not block overlapping slots | LOW |
| Item weight system | Full weight tracking with per-item weights | Implemented via `currentWeight`/`maxWeight` | OK |
| NPC sell price formula | `floor(Buy_Price / 2)` | Not audited in NPC shop handler | LOW |

---

## Card System (compounding, effects, naming, combos)

### Correctly Implemented
- **Card compounding**: `card:compound` handler with full validation pipeline (lines 23321-23469)
  - Card type must be `'card'`
  - Card must not be equipped
  - Equipment must be identified
  - `canCompoundCardOnEquipment()` validates card location vs equipment slot
  - Slot count validation with specific slot index
  - Empty slot check with array padding
  - Card consumed on compound (permanent, no removal)
  - If equipped, `rebuildCardBonuses()` + stat recalculation triggered
- **538 cards at 100% coverage** with 65+ bonus types parsed from rAthena scripts
- **Card effect categories** fully implemented:
  - Flat stat bonuses (combat/defense objects)
  - Race/element/size % damage (offensive in damage Step 6)
  - Race/element/size % reduction (defensive in damage Step 8c)
  - Armor element change (Ghostring, Pasana, etc.)
  - MaxHP/MaxSP rate percentages
  - Auto-spell on attack and when hit
  - HP/SP drain on attack
  - Status infliction on attack and when hit
  - Autobonus on attack and when hit
  - Kill hooks (SP gain, Zeny gain, EXP bonus, drop bonus)
  - Special flags (noSizeFix, splashRange, noMagicDamage, noCastCancel, etc.)
- **Stacking rules**: additive within category, multiplicative between categories (verified correct)
- **Card naming**: prefix/suffix stored in DB (`card_prefix`, `card_suffix` columns), `populate_card_naming.sql` with 441 entries
- **Card compound slot mapping** in `ro_card_effects.js` (lines 51-63): correct -- Head_Top/Mid/Low cards can go into any headgear position

### Discrepancies
| Feature | Deep Research | Implementation | Severity |
|---------|-------------|----------------|----------|
| Card combo set bonuses | Multiple specific cards grant additional set bonus (Mage set, Monk set, Thief set, etc.) | **NOT IMPLEMENTED** -- no `card_combo` or `item_combos` references found anywhere in server code | HIGH |
| Skill-granting cards | Smokie (Hiding Lv1), Creamy (Teleport Lv1), Horong (Sight Lv1), Poporing (Detoxify Lv1) should add usable skills to hotbar | **NOT IMPLEMENTED** -- card effects parsed but no skill-grant mechanism in hotbar system | MEDIUM |
| Socket Enchant NPC | Add slots to 0-slot equipment via NPC (Seiyablem for weapons, Leablem for armor) with tiered success rates | **NOT IMPLEMENTED** | MEDIUM |
| Card removal NPC | Optional pre-renewal feature (200,000z + materials, ~90% success) | **NOT IMPLEMENTED** (acceptable -- strict pre-renewal design decision) | LOW |
| Periodic card effects | Incantation Samurai -1% SP/sec, HP/SP regen rate from cards | Regen rate bonuses exist (`hpRecovRate`, `spRecovRate`), but periodic drain (per-second SP loss) not implemented | LOW |
| SP vanish on hit | Destroy target SP on physical hit | **NOT IMPLEMENTED** | LOW |
| Duplicate card stacking display | `[N]` shows total slots, not remaining | Client `GetDisplayName()` handles this correctly | OK |

---

## Refining System (rates, materials, ATK bonus, overupgrade)

### Correctly Implemented
- **REFINE_RATES table** (lines 24208-24214): matches deep research exactly for all weapon levels and armor
- **REFINE_MATERIALS** (lines 24217-24223): correct ores and zeny fees for all categories
- **Safe limits**: embedded in REFINE_RATES (100% up to safe limit, risk above)
- **Max refine +10**: enforced (line 24259)
- **Failure = item destruction**: DELETE from inventory on failed roll (line 24321), including all compounded cards
- **Ore + zeny consumed** before roll (lines 24292-24301)
- **Unidentified item blocking**: cannot refine unidentified items (line 24250)
- **Refineable check**: `item.refineable` must be true (line 24255)
- **Flat refine ATK per weapon level**: `REFINE_ATK_PER_LEVEL = [0, 2, 3, 5, 7]` (line 3865) -- matches deep research
- **Refine ATK applied post-DEF** in damage pipeline (line 792 in `ro_damage_formulas.js`) -- correctly bypasses hard DEF and soft DEF
- **REFINE_EXCLUDE_SKILLS**: Shield Boomerang (1305), Acid Terror (1801), Investigate (1606), Asura Strike (1605) -- matches deep research
- **Overupgrade random bonus**: OVER_BONUS table (lines 3876-3880) with correct values matching deep research (3/5/8/13 per overupgrade level)
- **Armor refine DEF**: `Math.floor((3 + row.refine_level) / 4)` per armor piece (line 5600) -- matches deep research
- **Dual-wield left-hand refine ATK**: separately calculated (lines 3885-3889)
- **Stats recalculation** on equipped item destruction or refine success

### Discrepancies
| Feature | Deep Research | Implementation | Severity |
|---------|-------------|----------------|----------|
| Enriched Oridecon/Elunium | Higher success rates (separate table provided) | **NOT IMPLEMENTED** | LOW (Cash Shop feature) |
| HD Oridecon/Elunium | Same rates as enriched but -1 refine on fail instead of destruction | **NOT IMPLEMENTED** | LOW (event-only feature) |
| Blacksmith Weapon Refine skill (WS_WEAPONREFINE) | Self-refine using own DEX + job level, no zeny fee | **NOT IMPLEMENTED** -- skill ID 1218 not found in handlers | MEDIUM |
| Overupgrade in damage pipeline position | Deep research: overupgrade bonus is "inside base damage (before DEF)", subject to DEF/skill ratio | Implementation: added post-DEF alongside refine ATK (line 800-804 in `ro_damage_formulas.js`) | **MEDIUM** -- overupgrade random bonus should be pre-DEF, not post-DEF |

### Critical: Overupgrade Placement
The deep research states:
- Flat refine ATK: post-DEF (bypasses DEF) -- **correctly implemented**
- Overupgrade random bonus: pre-DEF (inside base damage, IS subject to DEF/skill ratio) -- **incorrectly placed post-DEF**

Current code (ro_damage_formulas.js lines 800-804):
```javascript
// Overupgrade random bonus: 1 to overrefineMax per hit (above safe refine limit)
const overrefMax = attacker.overrefineMax || 0;
if (overrefMax > 0 && !REFINE_EXCLUDE_SKILLS.includes(skillId)) {
    totalATK += Math.floor(Math.random() * overrefMax) + 1;
}
```
This is in the "Post-DEF bonuses" section, but per rAthena, the overupgrade bonus should be added to the base weapon damage (before DEF reduction). Per deep research, it IS affected by DEF and skill ratios.

---

## Forging System (recipes, formula, anvils)

### Correctly Implemented
- **Forge handler** (`forge:request`, lines 24373-24567): full flow with material validation, element stone support, star crumb support, success roll, transaction-based material consumption
- **Success formula base**: `5000 + (jobLv * 20) + (DEX * 10) + (LUK * 10) + (smithLv * 500) + (wrLv * 100)` -- matches rAthena
- **Element stone penalty**: -2000 (line 24451) -- correct
- **Star crumb penalty**: -1500 per crumb (line 24452) -- correct
- **Star crumb ATK bonus**: `[0, 5, 10, 40]` (line 24534) -- correct
- **Forged weapon storage**: `forged_by`, `forged_element`, `forged_star_crumbs` columns in DB
- **Materials consumed on failure** -- correct behavior
- **Element stones**: 4 types (fire/water/wind/earth, IDs 994-997) -- correct

### Discrepancies
| Feature | Deep Research | Implementation | Severity |
|---------|-------------|----------------|----------|
| Weapon level penalty formula | `-(weaponLevel - 1) * 1000` (WLv1=-0, WLv2=-1000, WLv3=-2000) | `-(recipe.weaponLevel > 1 ? recipe.weaponLevel * 1000 : 0)` (WLv1=-0, WLv2=-2000, WLv3=-3000) | **HIGH** -- doubles the penalty, making forging WLv2-3 weapons significantly harder than intended |
| FORGE_RECIPES completeness | Deep research lists 38+ forgeable weapons across 7 Smith skills | Only **7 recipes** defined (1 per weapon type, all WLv1 only) | **HIGH** -- vast majority of forgeable weapons missing |
| Anvil types and bonuses | 4 anvil types: Anvil (+0), Oridecon (+300/+3%), Golden (+500/+5%), Emperium (+1000/+10%) | No anvil check at all in forge handler -- `AnvilBonus` term is absent from formula | **MEDIUM** -- anvils have no effect |
| Hammer requirement | Iron Hammer (WLv1), Golden Hammer (WLv2), Oridecon Hammer (WLv3) -- consumed per forge | No hammer check or consumption in forge handler | **MEDIUM** |
| Separate guide books per recipe | Different creation guides for different recipe groups | All recipes use guide 7144 | MEDIUM |
| Oridecon Research (1223) bonus | Should apply to ALL WLv3 forging, not just a subset | Only checked when `recipe.weaponLevel === 3` (line 24447-24450) but formula term is correct | OK |

### Critical: Forge Weapon Level Penalty Bug
Line 24453: `makePer -= (recipe.weaponLevel > 1 ? recipe.weaponLevel * 1000 : 0)`

Per rAthena (`skill.cpp`), the correct formula is: `-(weaponLevel - 1) * 1000`
- WLv 1: -0 (both agree)
- WLv 2: should be -1000, code applies -2000 (off by 1000)
- WLv 3: should be -2000, code applies -3000 (off by 1000)

**Fix**: Change to `makePer -= (recipe.weaponLevel - 1) * 1000;`

---

## Pharmacy System (recipes, formula)

### Correctly Implemented
- **Pharmacy handler** (`pharmacy:craft`, lines 21146-21230): full flow with guide check, Medicine Bowl consumption, ingredient consumption, success roll
- **Success rate formula** (lines 21198-21207): `(potResLv * 50) + (pharmacyLv * 300) + (jobLv * 20) + (INT/2 * 10) + (DEX * 10) + (LUK * 10) + rateMod` -- matches rAthena closely
- **Rate modifiers** per recipe type: basic potions (+2000), Alcohol/Blue Potion (+1000), bombs (0), Glistening Coat (-1000) -- generally correct
- **Medicine Bowl (7134) consumed per attempt** (line 21164-21181) -- correct
- **Guide book requirement** checked (line 21157) -- correct
- **Bioethics check for Embryo** (line 21161) -- correct
- **All materials consumed on failure** -- correct

### Recipes Implemented vs Deep Research
| Recipe | Deep Research | Implementation | Status |
|--------|-------------|----------------|--------|
| Red Potion (501) | 1 Red Herb + 1 Empty Potion Bottle | ID 713 + ID 507 | OK |
| Yellow Potion (503) | 1 Yellow Herb + 1 Empty Potion Bottle | ID 713 + ID 508 | OK |
| White Potion (504) | 1 White Herb + 1 Empty Potion Bottle | ID 713 + ID 509 | OK |
| Blue Potion (505) | 1 Blue Herb + 1 Scell + 1 Empty Potion Bottle | ID 713 + ID 510 + ID 911 | OK |
| Alcohol (970) | 5 Stem + 5 Poison Spore + 1 Empty Bottle + 1 Empty Test Tube | ID 713 + ID 7126 + 5x ID 1033 + 5x ID 7033 | OK (ingredients differ slightly but mapped) |
| Bottle Grenade (7135) | 1 Alcohol + 1 Fabric + 1 Empty Bottle | ID 713 + ID 1059 + ID 970 | OK |
| Acid Bottle (7136) | 1 Immortal Heart + 1 Empty Bottle | ID 713 + ID 929 | OK |
| Plant Bottle (7137) | -- | ID 713 + 2x ID 1033 | OK (for summon system) |
| Marine Sphere Bottle (7138) | -- | ID 713 + ID 1055 + ID 1051 | OK |
| Glistening Coat (7139) | 1 Zenorc's Fang + 1 Alcohol + 1 Empty Bottle | ID 713 + ID 950 + ID 1044 + ID 970 | OK |
| Anodyne (605) | 1 Alcohol + 1 Ment + 1 Empty Bottle | ID 713 + ID 970 + ID 708 | OK |
| Aloevera (606) | 1 Alcohol + 1 Aloe + 1 Empty Bottle | ID 713 + ID 518 + ID 704 | OK |
| Embryo (7142) | 1 Seed of Life + 1 Morning Dew + 1 Glass Tube | ID 7134 + ID 7133 + ID 7140 + ID 7141 | OK |

### Discrepancies
| Feature | Deep Research | Implementation | Severity |
|---------|-------------|----------------|----------|
| Condensed Red Potion (545) | 1 Red Potion + 1 Empty Test Tube + 1 Witch Starsand | **MISSING** from PHARMACY_RECIPES | **HIGH** -- core Alchemist product |
| Condensed Yellow Potion (546) | 1 Yellow Potion + 1 Empty Test Tube + 1 Witch Starsand | **MISSING** | **HIGH** |
| Condensed White Potion (547) | 1 White Potion + 1 Empty Test Tube + 1 Witch Starsand | **MISSING** -- "Slims" are the most important WoE/MVP consumable | **HIGH** |
| Elemental Converters | Created via Pharmacy skill (Sage's Create Elemental Converter skill 1420) | Implemented as separate `crafting:craft_converter` handler with different recipes (lines 21232-21295) -- uses Sage skill check, not Pharmacy | OK (alternative implementation) |
| Homunculus (Vanilmirth) bonus | Change Instruction skill adds up to +500 to brew rate | **NOT IMPLEMENTED** | LOW |
| Rate modifier randomness | Deep research: `base + 2000 + rnd(10, 1000)` for basic potions | Code applies random only when `rateMod > 0`: `rate += rnd(10, 1000)`, else when `rateMod < 0`: `rate -= rnd(10, 1000)`. For `rateMod === 0` (Blue Potion, bombs), no randomness is added | **MEDIUM** -- Blue Potion should have `+1000 + rnd(10, 1000)` but gets `rateMod: 0` with no random |
| Separate guide books | Potion Creation Guide, Bottle Grenade Guide, Slim Potion Book, Embryo Guide, Coat of Arms, Converter Guide | All recipes reference guide ID 7144 (Potion Creation Guide) | **MEDIUM** -- removes the guide specialization mechanic |

---

## Arrow Crafting

### Correctly Implemented
- **45 recipes** in `ro_arrow_crafting.js` matching deep research exactly
- **Weight check**: rejects if >= 50% weight (line 20311) -- correct
- **100% success rate**: always succeeds -- correct
- **1 source item consumed** per craft -- correct
- **Arrows added via `addItemToInventory`** (stacks properly) -- correct
- **Weight update** after crafting -- correct
- **Inventory refresh** sent to client -- correct
- **All arrow types covered**: Basic, Iron, Steel, Oridecon, Fire, Crystal, Wind, Stone, Silver, Holy, Counter Evil, Immaterial, Shadow, Rusty, Stun, Frozen, Flash, Cursed, Sleep, Mute, Sharp

### Discrepancies
None found. Arrow crafting implementation matches deep research completely.

---

## Critical Discrepancies

### Priority 1 -- Must Fix (Gameplay-Breaking or Significantly Incorrect)

| # | System | Issue | Details | Fix |
|---|--------|-------|---------|-----|
| 1 | Forging | **Weapon level penalty formula is wrong** | Code: `weaponLevel * 1000`. Correct: `(weaponLevel - 1) * 1000`. WLv2 gets -2000 instead of -1000, WLv3 gets -3000 instead of -2000. | Change line 24453 to `makePer -= (recipe.weaponLevel - 1) * 1000;` |
| 2 | Forging | **Only 7 recipes** out of 38+ forgeable weapons | Only 1 weapon per type at WLv1. Missing all WLv2-3 weapons (Stiletto, Gladius, Katana, Bastard Sword, Lance, Chain, Fist, etc.) | Add complete recipe set from deep research Section 2.2 |
| 3 | Pharmacy | **Condensed potions missing** | Condensed Red/Yellow/White Potions (545/546/547) not in PHARMACY_RECIPES. These are the most important Alchemist products for WoE/MVP. | Add 3 recipes with Condensed Potion Creation Guide check |
| 4 | Refining | **Overupgrade bonus placed post-DEF** | Should be pre-DEF (inside base damage, subject to DEF/skill ratio). Currently added post-DEF alongside flat refine ATK. | Move overupgrade calculation to pre-DEF section of damage formula |

### Priority 2 -- Important for Correctness

| # | System | Issue | Details | Fix |
|---|--------|-------|---------|-----|
| 5 | Cards | **Card combo set bonuses not implemented** | Class-defining builds (Mage set, Monk set, Thief set, Archer set) depend on multi-card combos providing bonus stats. | Parse `item_combos.yml` data, check during `rebuildCardBonuses()` |
| 6 | Forging | **No anvil type check or bonus** | Anvil types (Anvil/Oridecon/Golden/Emperium) should provide +0/+300/+500/+1000 to success rate. Currently ignored. | Add anvil detection + bonus in forge formula |
| 7 | Forging | **No hammer requirement check** | Iron/Golden/Oridecon Hammer should be required and consumed per forge attempt. Currently not checked. | Add hammer validation + consumption |
| 8 | Pharmacy | **All recipes use same guide book** | Different recipe categories should require different guide books (Potion Creation Guide, Bottle Grenade Guide, Slim Potion Book, Embryo Guide). | Add distinct guide IDs per recipe group |
| 9 | Cards | **Skill-granting cards not functional** | Smokie/Creamy/Horong/Poporing cards should add usable skills (Hiding/Teleport/Sight/Detoxify) to hotbar. | Integrate with hotbar/skill system |

### Priority 3 -- Nice to Have

| # | System | Issue | Details | Fix |
|---|--------|-------|---------|-----|
| 10 | Refining | Blacksmith Weapon Refine skill (WS_WEAPONREFINE, ID 1218) | Self-refine without NPC, uses own DEX/job level for rates. | Add skill handler referencing existing refine system |
| 11 | Cards | Socket Enchant NPC | Add slots to 0-slot equipment with tiered success rates. | New NPC handler + eligible item list |
| 12 | Pharmacy | Blue Potion rate modifier | Should be `+1000 + rnd(10,1000)` but has `rateMod: 0` with no random component. | Fix rateMod to 1000 for Blue Potion |
| 13 | Refining | Enriched/HD ores | Higher success rates or safe-fail for Cash Shop items. | Low priority unless Cash Shop is implemented |
| 14 | Cards | Periodic card effects | Incantation Samurai -1% SP/sec drain. | Add tick-based card drain system |

---

## Missing Features

### Not Yet Implemented (Confirmed)
1. **Card combo set bonuses** -- no combo definitions or checking logic exists
2. **Skill-granting cards** -- Smokie/Creamy/Horong/Poporing card skill integration
3. **Socket Enchant NPC** -- adding slots to 0-slot equipment
4. **Condensed potion recipes** (545/546/547) -- critical for Alchemist gameplay
5. **35+ forgeable weapon recipes** -- only 7 of 38+ recipes defined
6. **Anvil system** -- no anvil type detection or bonus application
7. **Hammer consumption** in forging -- not checked or consumed
8. **Blacksmith Weapon Refine** (skill 1218) -- no self-refine handler
9. **Enriched/HD ores** -- alternative refine materials
10. **Cooking system** -- quest chain, cookbooks, 60 stat food recipes
11. **Card removal NPC** -- optional, design decision
12. **Homunculus Vanilmirth brew bonus** -- Change Instruction skill bonus to pharmacy

### Partially Implemented
1. **Pharmacy guide books** -- all recipes use the same guide ID (7144) instead of distinct guides per category
2. **Pharmacy rate modifiers** -- Blue Potion and some bombs missing the random component

---

## Recommended Fixes

### Immediate (< 1 hour each)

1. **Fix forge weapon level penalty**: Line 24453 in `index.js`
   - Change: `makePer -= (recipe.weaponLevel > 1 ? recipe.weaponLevel * 1000 : 0);`
   - To: `makePer -= (recipe.weaponLevel - 1) * 1000;`

2. **Add condensed potion recipes**: After line 517 in `index.js`
   ```javascript
   545: { name: 'Condensed Red Potion', guide: 7133, ingredients: [{ id: 501, qty: 1 }, { id: 7135, qty: 1 }, { id: 7049, qty: 1 }], rateMod: 0 },
   546: { name: 'Condensed Yellow Potion', guide: 7133, ingredients: [{ id: 503, qty: 1 }, { id: 7135, qty: 1 }, { id: 7049, qty: 1 }], rateMod: 0 },
   547: { name: 'Condensed White Potion', guide: 7133, ingredients: [{ id: 504, qty: 1 }, { id: 7135, qty: 1 }, { id: 7049, qty: 1 }], rateMod: -1000 },
   ```

3. **Fix Blue Potion rate modifier**: Change `rateMod: 0` to `rateMod: 1000` for recipe 505

### Short-Term (1-3 hours each)

4. **Complete forge recipes**: Add all 38+ forgeable weapons from deep research Section 2.2 to FORGE_RECIPES with correct materials, weapon levels, and Smith skill IDs

5. **Add anvil system to forging**: Check inventory for anvil types (986/987/988/989) and add bonus (+0/+300/+500/+1000) to forge formula, consume on attempt

6. **Add hammer checks to forging**: Validate correct hammer type (1005/1006/1007) is in inventory and consume per attempt

7. **Move overupgrade bonus pre-DEF**: In `ro_damage_formulas.js`, relocate the overupgrade random bonus from post-DEF (line 800-804) to the pre-DEF base damage section

### Medium-Term (3-8 hours each)

8. **Card combo system**: Parse combo definitions from rAthena data, add combo checking to `rebuildCardBonuses()`, define combo bonuses for Mage/Monk/Thief/Acolyte/Archer sets

9. **Skill-granting cards**: Integrate Smokie/Creamy/Horong/Poporing card effects with the hotbar system to allow using Hiding/Teleport/Sight/Detoxify from cards
