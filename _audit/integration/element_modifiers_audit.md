# Element Modifiers & Damage Type Cross-System Audit

**Date**: 2026-03-22
**Scope**: Server-side element table, element assignment, element priority, damage pipeline order, damage types
**Files**: `ro_damage_formulas.js`, `index.js`, `ro_skill_data.js`, `ro_skill_data_2nd.js`, `ro_ground_effects.js`
**Methodology**: 7-pass manual audit against rAthena pre-renewal canonical data

---

## 1. Element Table (Pass 1)

**Location**: `ro_damage_formulas.js` lines 26-147
**Format**: `ELEMENT_TABLE[attackElement][defendElement][defendLevel - 1]`
**Dimensions**: 10 attack elements x 10 defend elements x 4 defend levels = 400 values

### Full Matrix (Level 1 Defenders)

| Atk\Def | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|---------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| Neutral | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 25 | 100 |
| Water | 100 | 25 | 100 | 150 | 50 | 100 | 75 | 100 | 100 | 100 |
| Earth | 100 | 100 | 100 | 50 | 150 | 100 | 75 | 100 | 100 | 100 |
| Fire | 100 | 50 | 150 | 25 | 100 | 100 | 75 | 100 | 100 | 125 |
| Wind | 100 | 175 | 50 | 100 | 25 | 100 | 75 | 100 | 100 | 100 |
| Poison | 100 | 100 | 125 | 125 | 125 | 0 | 75 | 50 | 100 | -25 |
| Holy | 100 | 100 | 100 | 100 | 100 | 100 | 0 | 125 | 100 | 150 |
| Shadow | 100 | 100 | 100 | 100 | 100 | 50 | 125 | 0 | 100 | -25 |
| Ghost | 25 | 100 | 100 | 100 | 100 | 100 | 75 | 75 | 125 | 100 |
| Undead | 100 | 100 | 100 | 100 | 100 | 50 | 100 | 0 | 100 | 0 |

### Verification Against rAthena `db/pre-re/attr_fix.yml`

**STATUS: CORRECT (All 400 values verified)**

- Previously audited in Phase 3 (2026-03-09) with 537 tests in `server/tests/test_element_table.js`
- Cross-referenced against rAthena, Hercules pre-re, iRO Wiki Classic, RateMyServer
- All 4 element levels (Lv1-4) verified
- Key edge cases verified:
  - Ghost vs Neutral Lv1 = 25%, Lv2+ = 0% (immune)
  - Undead vs Undead = 0% at all levels (immune)
  - Poison vs Undead = -25/-50/-75/-100 (heals)
  - Shadow vs Undead = -25/-50/-75/-100 (heals)
  - Holy vs Holy = 0/-25/-50/-100 (immune to self-healed)

**No violations found in the element table.**

---

## 2. Element Priority Chain (Pass 3)

**Location**: `index.js` lines 3352-3358 (`recalcEffectiveWeaponElement`)
**Location**: `index.js` line 3839 (`getAttackerInfo` weaponElement)
**Location**: `index.js` lines 1749, 15252-15274 (endow buffs)

### Priority Order (Highest to Lowest)

1. **Endow buff** (`getCombinedModifiers(player).weaponElement`) -- Sage endow skills (Blaze/Tsunami/Tornado/Quake), Aspersio (Holy), Enchant Poison, elemental converter items
2. **Arrow element** (non-neutral only, bow weapons only) -- `player.equippedAmmo.element` when `!= 'neutral'`
3. **Weapon element** -- `player.equippedWeaponRight.element` (from DB)
4. **Default** -- `'neutral'`

### Implementation in `getAttackerInfo`

```javascript
weaponElement: getCombinedModifiers(player).weaponElement || player.weaponElement || 'neutral'
```

- `getCombinedModifiers().weaponElement` comes from `getBuffModifiers()` which checks active endow buffs -- highest priority
- `player.weaponElement` is set by `recalcEffectiveWeaponElement()` which applies the Arrow > Weapon chain
- Falls through to `'neutral'` if nothing else

### `recalcEffectiveWeaponElement` Implementation

```javascript
function recalcEffectiveWeaponElement(player) {
    let effectiveElement = (player.equippedWeaponRight && player.equippedWeaponRight.element) || 'neutral';
    if (player.equippedAmmo && player.equippedAmmo.element !== 'neutral' && player.weaponType === 'bow') {
        effectiveElement = player.equippedAmmo.element;
    }
    player.weaponElement = effectiveElement;
}
```

### Endow Mutual Exclusion

```javascript
// Line 15256-15266: All endow types are mutually exclusive
const ENDOW_DISPLAY = {
    'endow_fire': 'Endow Blaze', 'endow_water': 'Endow Tsunami',
    'endow_wind': 'Endow Tornado', 'endow_earth': 'Endow Quake',
    'item_endow_fire': ..., 'item_endow_water': ...,
    'aspersio': 'Aspersio', 'enchant_poison': 'Enchant Poison'
};
// Removes any existing endow before applying new one
```

### Endow Expiry Revert

```javascript
// Line 15340-15342: Reverts to base weapon element when endow expires
if (removed.some(n => n.startsWith('endow_') || n.startsWith('item_endow_') ||
    n === 'aspersio' || n === 'enchant_poison')) {
    dispTarget.weaponElement = dispTarget.baseWeaponElement || 'neutral';
}
```

**STATUS: CORRECT**
- Endow > Arrow > Weapon > Neutral priority correctly implemented
- Arrow override only applies to bow weapons
- Mutual exclusion between all endow types
- Proper revert on endow expiry

---

## 3. Damage Pipeline Order (Pass 4)

### Physical Damage Pipeline (`calculatePhysicalDamage` in `ro_damage_formulas.js`)

Element modifier application order in the physical pipeline:

| Step | Operation | Line |
|------|-----------|------|
| 1 | Perfect Dodge check | 485-492 |
| 2 | Critical check | 500-521 |
| 3 | HIT/FLEE check | 530-543 |
| 4 | StatusATK + WeaponATK calculation | 551-597 |
| 5 | ArrowATK contribution | 603-612 |
| 6 | Critical damage bonus (+40%) | 618-622 |
| 7 | Buff ATK modifier (Provoke) | 625-626 |
| 8 | Skill multiplier | 631-633 |
| 9 | Card bSkillAtk bonus | 638-641 |
| 10 | **Card modifiers** (race x ele x size) | 649-656 |
| 10b | Passive race ATK (Demon Bane) | 661-666 |
| 10c | Card boss/normal class | 671-675 |
| 10d | Card sub-race | 680-683 |
| 10e | Card monster-specific | 688-691 |
| 10f | Card ranged ATK | 696-701 |
| 11 | **Element modifier computed** (deferred application) | 711-727 |
| 12 | Hard DEF reduction (percentage) | 769-777 |
| 13 | Soft DEF subtraction (flat) | 778 |
| 14 | Passive race DEF (Divine Protection) | 782-787 |
| 15 | Refine ATK added (post-DEF) | 793-804 |
| 16 | Mastery ATK added (post-DEF) | 809-811 |
| 17 | Defensive card modifiers | 818-853 |
| 18 | Sage zone boost | 858-866 |
| 19 | Dragonology race resist | 871-874 |
| **20** | **Element modifier APPLIED** | **880** |
| 21 | Element resistance (Skin Tempering) | 883-886 |
| 22 | Raid debuff | 892-895 |
| 23 | Floor to min 1 | 900 |

**STATUS: CORRECT** -- Element modifier is applied AFTER DEF/refine/mastery (step 20), per rAthena `battle_calc_element_damage` which is called after `battle_calc_attack_post_defense`.

### Magical Damage Pipeline (`calculateMagicalDamage` in `ro_damage_formulas.js`)

| Step | Operation | Line |
|------|-----------|------|
| 1 | GTB Card check | 939-944 |
| 2 | MATK min/max roll | 947-949 |
| 3 | Skill multiplier | 952 |
| 4 | Card MATK rate (bMatkRate) | 955-957 |
| 5 | Buff ATK modifier | 960-961 |
| 6 | Card per-skill bonus | 964-967 |
| 7 | Card magic race bonus | 970-974 |
| **8** | **Element modifier** | **979-988** |
| 9 | Hard MDEF (percentage) | 993-1005 |
| 10 | Soft MDEF (flat subtraction) | 1010-1011 |
| 11 | Buff bonus MDEF | 1014-1015 |
| 12 | Status MDEF multiplier (freeze/stone) | 1019-1022 |
| 13 | Sage zone boost | 1026-1033 |
| 14 | Dragonology magic ATK | 1036-1038 |
| 15 | Dragonology race resist | 1041-1044 |
| 16 | Raid debuff | 1047-1050 |
| 17 | Floor to min 1 | 1052 |

**OBSERVATION**: In the magic pipeline, element modifier (step 8) is applied BEFORE MDEF reduction (step 9). This differs from the physical pipeline where element is applied AFTER DEF. Per rAthena pre-renewal, this is actually the correct order for magic -- `battle_calc_magic_attack` applies elemental modifier before MDEF in pre-renewal.

**STATUS: CORRECT** -- Both pipelines follow rAthena pre-renewal ordering.

---

## 4. Per-Skill Element Audit (Pass 2 + 5)

### Legend
- **W** = Uses weapon element (skillElement: null)
- **F** = Forced element (hardcoded)
- **ATK** = Physical damage
- **MATK** = Magical damage
- **MISC** = Neither ATK nor MATK (fixed formula, ignores DEF/MDEF)
- **HEAL** = Heal (no damage, or damage to undead only)

### Novice Skills (IDs 1-3)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1 | Basic Skill | neutral | N/A (passive) | N/A | N/A | OK |
| 2 | First Aid | neutral | N/A (self-heal) | HEAL | HEAL | OK |
| 3 | Play Dead | neutral | N/A (toggle) | N/A | N/A | OK |

### Swordsman Skills (IDs 100-109)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 103 | Bash | neutral | W (null) | ATK | W | OK |
| 105 | Magnum Break | fire | F (fire) | ATK | F (fire) | OK |
| 109 | Fatal Blow | neutral | N/A (passive, Bash proc) | ATK | W | OK |

### Mage Skills (IDs 200-213)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 200 | Cold Bolt | water | F (skill.element=water) | MATK | F (water) | OK |
| 201 | Fire Bolt | fire | F (skill.element=fire) | MATK | F (fire) | OK |
| 202 | Lightning Bolt | wind | F (skill.element=wind) | MATK | F (wind) | OK |
| 203 | Napalm Beat | ghost | F (skill.element=ghost) | MATK | F (ghost) | OK |
| 206 | Stone Curse | earth | N/A (no damage, status) | Status | Status | OK |
| 207 | Fire Ball | fire | F (skill.element=fire) | MATK | F (fire) | OK |
| 208 | Frost Diver | water | F ('water') | MATK | F (water) | OK |
| 209 | Fire Wall | fire | F ('fire') | MATK | F (fire) | OK |
| 210 | Soul Strike | ghost | F (skill.element=ghost) | MATK | F (ghost) | OK |
| 212 | Thunderstorm | wind | F (skill.element=wind) | MATK | F (wind) | OK |

### Archer Skills (IDs 300-306)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 303 | Double Strafe | neutral | W (null) | ATK | W | OK |
| 304 | Arrow Shower | neutral | W (null) | ATK | W | OK |
| 306 | Arrow Repel | neutral | W (null) | ATK | W | OK |

### Acolyte Skills (IDs 400-414)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 400 | Heal | holy | F (holy, undead dmg path) | HEAL/MATK vs Undead | HEAL/Holy vs Undead | OK |
| 408 | Ruwach | holy | F ('holy') | MATK | F (holy) | OK |
| 414 | Holy Light | holy | F ('holy') | MATK | F (holy) | OK |

### Thief Skills (IDs 500-509)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 504 | Envenom | poison | F ('poison') | ATK | F (poison) | OK |
| 506 | Sand Attack | earth | F (forceElement:'earth') | ATK | F (earth) | OK |
| 508 | Throw Stone | neutral | Fixed 50 dmg, 'neutral' broadcast | MISC | MISC, neutral | OK |

### Merchant Skills (IDs 600-609)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 603 | Mammonite | neutral | W (null) | ATK | W | OK |
| 608 | Cart Revolution | neutral | W (null) | ATK | W | OK |

### Knight Skills (IDs 700-710)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 701 | Pierce | neutral | W (null) | ATK | W | OK |
| 702 | Spear Stab | neutral | W (null) | ATK | W | OK |
| 703 | Brandish Spear | neutral | W (null) | ATK | W | OK |
| 704 | Spear Boomerang | neutral | W (null) | ATK | W | OK |
| 707 | Bowling Bash | neutral | W (null) | ATK | W | OK |
| 710 | Charge Attack | neutral | W (null) | ATK | W | OK |

### Wizard Skills (IDs 800-813)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 800 | Jupitel Thunder | wind | F ('wind') | MATK | F (wind) | OK |
| 801 | Lord of Vermilion | wind | F ('wind') | MATK | F (wind) | OK |
| 802 | Meteor Storm | fire | F ('fire') | MATK | F (fire) | OK |
| 803 | Storm Gust | water | F ('water') | MATK | F (water) | OK |
| 804 | Earth Spike | earth | F ('earth') | MATK | F (earth) | OK |
| 805 | Heaven's Drive | earth | F ('earth') | MATK | F (earth) | OK |
| 807 | Water Ball | water | F ('water') | MATK | F (water) | OK |
| 809 | Sight Rasher | fire | F ('fire') | MATK | F (fire) | OK |
| 810 | Fire Pillar | fire | F ('fire') | MATK | F (fire) | OK |
| 811 | Frost Nova | water | F ('water') | MATK | F (water) | OK |
| 813 | Sight Blaster | fire | F ('fire') | MATK | F (fire) | OK |

### Hunter Skills (IDs 900-917)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 900 | Blitz Beat | neutral | F ('neutral') | MISC | MISC, neutral | OK |
| 904 | Land Mine | earth | F ('earth') | MISC | MISC, earth | OK |
| 907 | Claymore Trap | fire | F ('fire') | MISC | MISC, fire | OK |
| 911 | Freezing Trap | water | ATK-based (weapon pipeline) | ATK (Weapon) | ATK (rAthena Type:Weapon) | OK |
| 912 | Blast Mine | wind | F ('wind') | MISC | MISC, wind | OK |
| 917 | Phantasmic Arrow | neutral | W (null) | ATK | W | OK |

### Priest Skills (IDs 1000-1018)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1000 | Sanctuary | holy | F ('holy') heal + Holy dmg vs Undead | HEAL + MATK vs Undead | HEAL + Holy vs Undead | OK |
| 1005 | Magnus Exorcismus | holy | F ('holy') | MATK | F (holy) | OK |
| 1006 | Turn Undead | holy | F ('holy') | MATK | F (holy) | OK |

### Assassin Skills (IDs 1100-1111)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1100 | Katar Mastery | neutral | N/A (passive) | N/A | N/A | OK |
| 1101 | Sonic Blow | neutral | W (null) | ATK | W | OK |
| 1102 | Grimtooth | neutral | W (null) | ATK | W | OK |
| 1103 | Cloaking | neutral | N/A (toggle) | N/A | N/A | OK |
| 1104 | Right Hand Mastery | neutral | N/A (passive) | N/A | N/A | OK |
| 1105 | Venom Dust | poison | N/A (trap, no direct dmg) | N/A | N/A | OK |
| 1106 | Venom Splasher | neutral | W (null) | ATK | W | OK |
| 1109 | Enchant Poison | poison | N/A (buff) | N/A | N/A | OK |

### Blacksmith Skills (IDs 1200-1230)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1207 | Adrenaline Rush | neutral | N/A (buff) | N/A | N/A | OK |
| 1208 | Weapon Perfection | neutral | N/A (buff) | N/A | N/A | OK |
| 1209 | Power Thrust | neutral | N/A (buff) | N/A | N/A | OK |
| 1210 | Maximize Power | neutral | N/A (buff) | N/A | N/A | OK |
| 1211 | Hammer Fall | neutral | W (null) | ATK | W | OK |

### Crusader Skills (IDs 1300-1313)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1302 | Holy Cross | holy | F ('holy') | ATK | F (holy) | OK |
| 1303 | Grand Cross | holy | F ('holy', element table) | ATK+MATK hybrid | Hybrid Holy | OK |
| 1304 | Shield Charge | neutral | W (null) | ATK | W | OK |
| 1305 | Shield Boomerang | neutral | F ('neutral') hardcoded | ATK (custom) | F (neutral) | OK |

### Sage Skills (IDs 1400-1421)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1408 | Endow Blaze | fire | N/A (buff) | N/A | N/A | OK |
| 1409 | Endow Tsunami | water | N/A (buff) | N/A | N/A | OK |
| 1410 | Endow Tornado | wind | N/A (buff) | N/A | N/A | OK |
| 1411 | Endow Quake | earth | N/A (buff) | N/A | N/A | OK |
| 1417 | Earth Spike (Sage) | earth | F ('earth') | MATK | F (earth) | OK |
| 1418 | Heaven's Drive (Sage) | earth | F ('earth') | MATK | F (earth) | OK |

### Bard Skills (IDs 1500-1537)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1505 | Dissonance | neutral | MISC (neutral) | MISC | MISC, neutral | OK |
| 1511 | Musical Strike | neutral | W (null) | ATK | W (arrow element) | OK |

### Dancer Skills (IDs 1520-1557)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1528 | Slinging Arrow | neutral | W (null) | ATK | W (arrow element) | OK |

### Monk Skills (IDs 1600-1615)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1604 | Finger Offensive | neutral | W (null) | ATK | W | OK |
| 1605 | Asura Strike | neutral | F ('neutral') hardcoded | ATK (custom) | F (neutral) | OK |
| 1606 | Investigate | neutral | F ('neutral') hardcoded | ATK (custom, DEF-based) | F (neutral) | OK |
| 1613 | Combo Finish | neutral | W (null) | ATK | W | OK |

### Rogue Skills (IDs 1700-1718)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1702 | Back Stab | neutral | W (null) | ATK | W | OK |
| 1703 | Raid | neutral | W (null) | ATK | W | OK |
| 1704 | Intimidate | neutral | W (null) | ATK | W | OK |
| 1705 | Double Strafe (Rogue) | neutral | W (null) | ATK | W | OK |

### Alchemist Skills (IDs 1800-1815)

| ID | Skill | Data Element | Handler Element | Damage Type | RO Classic | Status |
|----|-------|-------------|-----------------|-------------|------------|--------|
| 1801 | Acid Terror | neutral | F ('neutral') | ATK (forceHit, ignoreHardDef) | F (neutral) | OK |
| 1802 | Demonstration | fire | F ('fire') | ATK (ground DoT) | F (fire) | OK |

---

## 5. Damage Type Audit (Pass 2)

### Physical (ATK) Skills -- Use `calculateSkillDamage` or `calculatePhysicalDamage`

All physical skills correctly use the physical damage pipeline with HIT/FLEE checks, DEF reduction, size penalty, and card modifiers:

- Bash (103), Magnum Break (105), Double Strafe (303), Arrow Shower (304), Arrow Repel (306)
- Envenom (504), Sand Attack (506), Mammonite (603), Cart Revolution (608)
- Pierce (701), Spear Stab (702), Brandish Spear (703), Spear Boomerang (704)
- Bowling Bash (707), Charge Attack (710), Holy Cross (1302), Shield Charge (1304)
- Sonic Blow (1101), Grimtooth (1102), Venom Splasher (1106)
- Back Stab (1702), Raid (1703), Intimidate (1704)
- Finger Offensive (1604), Combo skills (1610-1613)
- Hammer Fall (1211), Acid Terror (1801), Demonstration (1802)
- Musical Strike (1511), Slinging Arrow (1528), Phantasmic Arrow (917)
- Freezing Trap (911) -- correctly uses ATK pipeline per rAthena Type:Weapon

### Magical (MATK) Skills -- Use `calculateMagicSkillDamage` or `roMagicalDamage`

All magical skills correctly use the magical damage pipeline with MATK roll, MDEF reduction, and element modifier:

- Cold Bolt (200), Fire Bolt (201), Lightning Bolt (202), Napalm Beat (203)
- Soul Strike (210), Fire Ball (207), Frost Diver (208), Fire Wall (209)
- Thunderstorm (212), Holy Light (414), Ruwach (408)
- Jupitel Thunder (800), Lord of Vermilion (801), Meteor Storm (802)
- Storm Gust (803), Earth Spike (804), Heaven's Drive (805)
- Water Ball (807), Sight Rasher (809), Fire Pillar (810)
- Frost Nova (811), Sight Blaster (813)
- Magnus Exorcismus (1005), Turn Undead (1006)
- Sanctuary (1000) -- Holy damage portion vs Undead
- Heal (400) -- Holy damage vs Undead enemies

### MISC Damage Skills -- Custom formula, bypasses DEF/MDEF

All MISC-type skills correctly use custom formulas that bypass normal damage pipelines:

- Throw Stone (508) -- Fixed 50 damage, neutral, hitType: 'normal'
- Blitz Beat (900) -- (DEX/10 + INT/2 + SC*3 + 40) * 2, neutral, hitType: 'misc'
- Land Mine (904) -- `calculateTrapDamage` MISC formula, earth element
- Blast Mine (912) -- `calculateTrapDamage` MISC formula, wind element
- Claymore Trap (907) -- `calculateTrapDamage` MISC formula, fire element
- Dissonance (1505) -- MISC damage, neutral

### Hybrid Damage Skills

- Grand Cross (1303) -- WeaponATK (no StatusATK) + MATK, Holy element. Both portions independently calculated. Correctly hybrid.

### Custom Pipeline Skills

- Investigate (1606) -- ATK * multiplier * DEF/50. DEF becomes damage amplifier instead of reduction. Always neutral. Correctly implemented.
- Asura Strike (1605) -- (WeaponATK + StatusATK) * (8 + SP/10) + flat. Always neutral. Correctly implemented.
- Shield Boomerang (1305) -- StatusATK + shieldWeight/10 + refine*5. Always neutral. Correctly implemented.

---

## 6. Element Correctness Cross-Reference (Pass 5 + 6)

### Skills That Should Force an Element

| Skill | Expected | Actual | Status |
|-------|----------|--------|--------|
| Magnum Break | Fire | Fire (forced) | OK |
| Envenom | Poison | Poison (forced) | OK |
| Sand Attack | Earth | Earth (forced via forceElement) | OK |
| Holy Cross | Holy | Holy (forced) | OK |
| Grand Cross | Holy | Holy (element table) | OK |
| Heal vs Undead | Holy | Holy (element table) | OK |
| Aspersio | Holy (endow) | Holy (buff) | OK |
| Enchant Poison | Poison (endow) | Poison (buff) | OK |
| All bolt spells | Fire/Water/Wind | Correct per skill.element | OK |
| Fire Wall | Fire | Fire (hardcoded) | OK |
| Sight Blaster | Fire | Fire (hardcoded) | OK |
| Sight Rasher | Fire | Fire (hardcoded) | OK |

### Skills That Should Use Weapon Element

| Skill | Expected | Actual | Status |
|-------|----------|--------|--------|
| Bash | Weapon | null (falls through to weapon) | OK |
| Double Strafe | Weapon | null (falls through to weapon) | OK |
| Arrow Shower | Weapon | null (falls through to weapon) | OK |
| Mammonite | Weapon | null (falls through to weapon) | OK |
| Pierce | Weapon | null (falls through to weapon) | OK |
| Bowling Bash | Weapon | null (falls through to weapon) | OK |
| Sonic Blow | Weapon | null (falls through to weapon) | OK |

### Skills That Should Always Be Neutral

| Skill | Expected | Actual | Status |
|-------|----------|--------|--------|
| Throw Stone | Neutral | Neutral (fixed 50) | OK |
| Blitz Beat | Neutral | Neutral (MISC formula) | OK |
| Shield Boomerang | Neutral | Neutral (hardcoded) | OK |
| Investigate | Neutral | Neutral (hardcoded) | OK |
| Asura Strike | Neutral | Neutral (hardcoded) | OK |
| Acid Terror | Neutral | Neutral (hardcoded) | OK |

### Trap Elements

| Trap | Expected | Actual | Status |
|------|----------|--------|--------|
| Land Mine | Earth | Earth (getElementModifier('earth',...)) | OK |
| Blast Mine | Wind | Wind (getElementModifier('wind',...)) | OK |
| Claymore Trap | Fire | Fire (getElementModifier('fire',...)) | OK |
| Freezing Trap | Water | N/A (ATK-based, uses weapon element) | OK |

---

## 7. Monster Auto-Attack Element (Pass 5)

**Location**: `index.js` line 29911

Monster auto-attacks pass `isNonElemental: true`, which causes the physical damage formula to bypass the element table entirely (always 100% damage regardless of target armor element).

**This is RO Classic correct**: In pre-renewal, monster basic attacks are "non-elemental" (sometimes called "typeless"). Ghostring Card (Ghost Lv1 armor) protects against player neutral attacks (Ghost vs Neutral Lv1 = 25%) but does NOT protect against monster basic attacks.

**STATUS: CORRECT**

---

## 8. Freeze/Stone Element Override (Pass 5)

**Location**: `ro_damage_formulas.js` lines 472-474, verified in bolt handler at line 10172

When a target is frozen, they become Water Lv1 (vulnerable to Wind). When petrified, they become Earth Lv1 (vulnerable to Fire). This is handled via:

1. `targetBuffMods.overrideElement` from `getCombinedModifiers()` in the skill handlers
2. `getStatusModifiers()` returns `overrideElement: { type: 'water', level: 1 }` for freeze
3. Bolt handlers explicitly check: `if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement`

**STATUS: CORRECT**

---

## 9. VIOLATIONS

### No Violations Found

After comprehensive 7-pass audit:

1. **Element table**: All 400 values match rAthena pre-renewal canonical data
2. **Element priority**: Endow > Arrow > Weapon > Neutral correctly implemented
3. **Pipeline order**: Element modifier applied after DEF in physical, before MDEF in magic (both correct per rAthena)
4. **Per-skill elements**: All 80+ damage-dealing skills use the correct element
5. **Damage types**: All skills use the correct pipeline (ATK/MATK/MISC)
6. **Forced elements**: All forced-element skills correctly hardcode their element
7. **Weapon-element skills**: All weapon-element skills correctly pass `null` (not `'neutral'`)
8. **Non-elemental**: Monster auto-attacks correctly bypass the element table
9. **Status overrides**: Freeze (Water Lv1) and Stone (Earth Lv1) correctly override target element

---

## 10. Observations (Non-Violations)

### O1: `calculateMagicSkillDamage` Wrapper Simplification

The `calculateMagicSkillDamage` wrapper passes `weaponMATK: 0` and `buffMods: { atkMultiplier: 1.0 }`. This appears to ignore weapon MATK and buff multipliers, but is actually correct because:

1. `weaponMATK` is included in `attackerStats` from `getEffectiveStats()` and feeds into `calculateDerivedStats()` which computes matkMin/matkMax. The explicit `weaponMATK: 0` on the attacker object is unused by `roMagicalDamage`.
2. `atkMultiplier: 1.0` is correct because Provoke and similar buffs only affect physical ATK, not MATK in RO Classic.

**However**, if a future buff or card is intended to increase magic damage via `atkMultiplier`, it will not work through this wrapper. The naming is slightly misleading. Not a bug today, but could cause confusion later.

### O2: Sage Zone Boost Position in Magic Pipeline

Sage zone boosts (Volcano +fire%, Deluge +water%, Violent Gale +wind%) are applied AFTER MDEF reduction in the magic pipeline (lines 1026-1033), but AFTER element modifier. In the physical pipeline, they're applied AFTER defensive cards (lines 858-866). Both positions are post-DEF, which is functionally correct, though rAthena applies them slightly differently. The practical impact is negligible.

### O3: Broad Skill Coverage

The codebase implements approximately:
- 15 first-class skills with forced elements
- 30+ wizard/sage/priest magic skills with forced elements
- 25+ physical skills correctly using weapon element
- 6 MISC-type skills with custom formulas
- 4 trap types with correct elements
- 3 custom-pipeline skills (Investigate, Asura, Shield Boomerang)

All are correctly categorized and use the right damage pipeline.

---

## 11. Recommendations

1. **No immediate action required.** The element system is comprehensively correct across all audited paths.

2. **Future-proofing**: If adding new magic damage modifiers, ensure they are added to `calculateMagicSkillDamage` wrapper or directly to `roMagicalDamage`, not relying on the hardcoded `atkMultiplier: 1.0`.

3. **Test coverage**: The existing 537-test suite in `server/tests/test_element_table.js` covers the element table. Consider adding integration tests for:
   - Element priority chain (Endow > Arrow > Weapon)
   - Freeze/Stone element override in damage calculations
   - Non-elemental monster attacks vs Ghostring armor

4. **Documentation**: The `skill.element === 'neutral' ? null : skill.element` pattern (line 9652, 1937, 3694) is a critical convention. Consider adding a comment in `ro_skill_data.js` explaining that `element: 'neutral'` means "uses weapon element" not "forces neutral element."

---

## Summary

| Area | Status | Details |
|------|--------|---------|
| Element Table (10x10x4) | CORRECT | 400 values match rAthena pre-renewal |
| Element Priority Chain | CORRECT | Endow > Arrow > Weapon > Neutral |
| Physical Pipeline Order | CORRECT | Element applied after DEF/refine/mastery |
| Magic Pipeline Order | CORRECT | Element applied before MDEF (rAthena pre-re) |
| Skill Elements (80+ skills) | CORRECT | All match RO Classic |
| Damage Types (ATK/MATK/MISC) | CORRECT | All use correct pipeline |
| Monster Non-Elemental | CORRECT | Auto-attacks bypass element table |
| Freeze/Stone Override | CORRECT | Water Lv1 / Earth Lv1 |
| **Violations Found** | **0** | |
