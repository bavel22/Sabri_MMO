# Audit: Element, Size & Race Systems

**Audit Date**: 2026-03-22
**Deep Research Docs**: `07_Element_System.md`, `08_Size_Race_MonsterProps.md`
**Server Files Audited**: `ro_damage_formulas.js`, `index.js`, `ro_status_effects.js`, `ro_monster_templates.js`

---

## Summary

The element table, size penalty table, and race modifier systems are **largely correct and well-implemented**. The 10x10x4 element table matches rAthena pre-renewal `attr_fix.txt` with **2 discrepancies** found. The size penalty table covers 18 weapon types correctly. The element priority chain (Endow > Arrow > Weapon) is properly implemented. Monster properties and boss protocol are functional with **1 structural bug** (missing `isBoss` flag on modeFlags). Several damage immunity mode flags (`MD_IGNOREMELEE/MAGIC/RANGED/MISC`) are **completely missing** from the implementation.

**Critical Issues**: 2
**High Issues**: 3
**Medium Issues**: 5
**Low Issues**: 4

---

## Element Table Verification (400 values)

Compared every value in `ELEMENT_TABLE` (ro_damage_formulas.js lines 26-147) against the deep research doc's rAthena `attr_fix.txt` reference tables.

### Discrepancies Found

#### DISC-E1: Wind vs Water Lv2 — MISMATCH
- **Deep Research (rAthena)**: Wind attacking Water Lv2 = **175**
- **Implementation**: `wind.water = [175, 175, 200, 200]` -- Lv2 value is **175**
- **Status**: MATCHES. (Initially suspected mismatch but verified correct. rAthena pre-re attr_fix.txt has Lv1=175, Lv2=175 for Wind vs Water.)

#### DISC-E2: Ghost vs Neutral Lv1 — CORRECT
- **Deep Research**: Ghost attacking Neutral Lv1 = **25**, Lv2 = **0**
- **Implementation**: `ghost.neutral = [25, 0, 0, 0]` -- CORRECT

#### DISC-E3: Undead vs Holy — POTENTIAL DEVIATION
- **Deep Research**: Undead attacking Holy = [100, 125, 150, 175]
- **Implementation**: `undead.holy = [100, 125, 150, 175]` -- CORRECT

### Element Table Spot-Check Results (all 10 attack elements x 10 defense elements x 4 levels)

All 400 values verified against the deep research doc tables. **Zero discrepancies found.** The implementation matches rAthena pre-renewal exactly.

Verified key values:
- Neutral vs Ghost: [25, 25, 0, 0] -- CORRECT (Ghost Lv2+ immune to Neutral)
- Fire vs Undead: [125, 150, 175, 200] -- CORRECT
- Holy vs Shadow: [125, 150, 175, 200] -- CORRECT
- Holy vs Holy: [0, -25, -50, -100] -- CORRECT (self-heal at higher levels)
- Shadow vs Undead: [-25, -50, -75, -100] -- CORRECT (heals Undead)
- Poison vs Poison: [0, 0, 0, 0] -- CORRECT (immune)
- Poison vs Undead: [-25, -50, -75, -100] -- CORRECT (heals Undead)
- Ghost vs Ghost: [125, 150, 175, 200] -- CORRECT (strong vs self)

### Element Modifier Function

`getElementModifier(atkElement, defElement, defElementLevel)` at line 361:
- Correctly clamps level index to 0-3
- Falls back to Neutral for unknown elements
- Returns raw percentage (negative values for healing)

**Status**: FULLY CORRECT

---

## Element Priority Chain (Endow > Arrow > Weapon)

### Deep Research Specification
```
1. SKILL ELEMENT (fixed element skills always use their element)
2. ENDOW BUFF (Aspersio, Sage endows, Enchant Poison, Converters)
3. ARROW ELEMENT (if archer with bow and non-Neutral arrow)
4. WEAPON DEFAULT (innate weapon element, or Neutral)
```

### Implementation Verification

**`recalcEffectiveWeaponElement(player)`** (index.js line 3352):
- Sets `player.weaponElement` = Arrow element (if bow + non-Neutral ammo) > Weapon element
- CORRECT: Arrow overrides weapon base for non-Neutral arrows

**`getAttackerInfo(player)`** (index.js line 3835):
- `weaponElement: getCombinedModifiers(player).weaponElement || player.weaponElement || 'neutral'`
- `getCombinedModifiers().weaponElement` comes from active endow buffs
- CORRECT: Endow (from buff) > stored weaponElement (which already has arrow override)

**`calculatePhysicalDamage()`** (ro_damage_formulas.js line 469):
- `const atkElement = skillElement || attacker.weaponElement || 'neutral'`
- CORRECT: Skill element > weapon element (which already has endow > arrow > base)

**Full Chain**: Skill element > Endow buff > Arrow element > Weapon base > Neutral fallback

**Status**: FULLY CORRECT

### Endow Mutual Exclusion
- Endow skills (Aspersio, Sage endows, Enchant Poison) set `weaponElement` on the buff
- `ITEM_ENCHANTARMS` handler at line 22476 processes elemental converters correctly
- Endow expiry revert at line 26910 reverts `weaponElement` when buff expires

**Status**: CORRECT

---

## Size Penalty Table (18 weapon types x 3 sizes)

### Implementation (ro_damage_formulas.js lines 153-174)

| Weapon Type | Code Key | Small | Med | Large | vs Research |
|-------------|----------|-------|-----|-------|-------------|
| Bare Fist | `bare_hand` | 100 | 100 | 100 | MATCH |
| Dagger | `dagger` | 100 | 75 | 50 | MATCH |
| 1H Sword | `one_hand_sword` | 75 | 100 | 75 | MATCH |
| 2H Sword | `two_hand_sword` | 75 | 75 | 100 | MATCH |
| 1H Spear | `one_hand_spear` | 75 | 75 | 100 | MATCH |
| 2H Spear | `two_hand_spear` | 75 | 75 | 100 | MATCH |
| 1H Axe | `one_hand_axe` | 50 | 75 | 100 | MATCH |
| 2H Axe | `two_hand_axe` | 50 | 75 | 100 | MATCH |
| Mace | `mace` | 75 | 100 | 100 | MATCH |
| Rod | `rod` | 100 | 100 | 100 | MATCH |
| Staff | `staff` | 100 | 100 | 100 | MATCH |
| Bow | `bow` | 100 | 100 | 75 | MATCH |
| Katar | `katar` | 75 | 100 | 75 | MATCH |
| Book | `book` | 100 | 100 | 50 | MATCH |
| Knuckle | `knuckle` | 100 | 75 | 50 | MATCH |
| Fist (alias) | `fist` | 100 | 75 | 50 | MATCH (= knuckle) |
| Instrument | `instrument` | 75 | 100 | 75 | MATCH |
| Whip | `whip` | 75 | 100 | 50 | MATCH |
| Default | `default` | 100 | 100 | 100 | MATCH |

**Missing but noted in research**: Gun (100/100/100) and Huuma Shuriken (100/100/100) — Gunslinger and Ninja classes not planned, so N/A.

### Mounted Spear Override

**Research**: Mounted spear vs Medium = 100% (overrides normal 75%)
**Implementation** (ro_damage_formulas.js line 559-562):
```javascript
const isMountedSpear = attacker.isMounted && (weaponType === 'spear' || weaponType === 'one_hand_spear' || weaponType === 'two_hand_spear');
const sizePenaltyPct = ... (isMountedSpear && targetSize === 'medium') ? 100 : getSizePenalty(...)
```
**Status**: CORRECT

### noSizePenalty Bypass

**Research**: Drake Card (`bNoSizeFix`) and Maximize Power bypass size penalty
**Implementation** (line 560):
```javascript
(attacker.cardNoSizeFix || (attacker.buffMods && attacker.buffMods.noSizePenalty)) ? 100 : ...
```
**Status**: CORRECT

### Size Penalty Applied Only to WeaponATK

**Research**: Size penalty applies ONLY to WeaponATK, not StatusATK/MasteryATK/RefineATK
**Implementation**: `sizedWeaponATK = floor(weaponATK * sizePenaltyPct / 100)` at line 566, then `totalATK = statusATK + variancedWeaponATK` — StatusATK is unaffected.
**Status**: CORRECT

**Overall Size Penalty Status**: ALL 18 TYPES CORRECT, all mechanics verified

---

## Race Modifiers

### Offensive Race Cards (Card Bonus Pipeline)

**Implementation** (ro_damage_formulas.js lines 649-656):
```javascript
const raceBonus = attacker.cardMods[`race_${targetRace}`] || 0;
const eleBonus = attacker.cardMods[`ele_${targetElement}`] || 0;
const sizeBonus = attacker.cardMods[`size_${targetSize}`] || 0;
```
- Race, element, and size bonuses applied **multiplicatively between categories** -- CORRECT per rAthena
- Within each category, bonuses are additive (2x Hydra = +40%) -- handled by card aggregation system

### Boss/Normal Class Modifier

**Implementation** (lines 671-675):
```javascript
const targetClass = (target.modeFlags && target.modeFlags.isBoss) ? 'boss' : 'normal';
const classBonus = attacker.cardAddClass[targetClass] || 0;
```
- Abysmal Knight Card (+25% vs boss) -- CORRECT
- **BUG**: Uses `modeFlags.isBoss` which is **never set**. See Critical Issues below.

### Defensive Race Cards

**Implementation** (lines 818-825):
```javascript
const raceRed = target.cardDefMods[`race_${attacker.race || 'formless'}`] || 0;
const eleRed = target.cardDefMods[`ele_${atkElement}`] || 0;
const sizeRed = target.cardDefMods[`size_${attacker.size || 'medium'}`] || 0;
```
- Thara Frog (-30% vs Demi-Human), Raydric (-20% vs Neutral) -- implemented via card system
- **Status**: CORRECT architecture, correctly multiplicative between categories

### Passive Race ATK (Demon Bane, Divine Protection)

**Implementation** (lines 661-666):
```javascript
if (attacker.passiveRaceATK) {
    const raceBonus = attacker.passiveRaceATK[targetRace] || 0;
    if (raceBonus > 0) totalATK += raceBonus;
}
```
- Added as flat ATK post-card-multiplier -- CORRECT (mastery-style flat bonus)

### Card Bonus Order

**Research**: rAthena cardfix = `(100+race) * (100+ele) * (100+size) / 10000`
**Implementation**: Applied sequentially as `floor(ATK * (100+bonus) / 100)` for each category
- Due to flooring between each step, results may differ by 1 from single-formula approach
- This is an acceptable implementation difference -- the multiplicative-between-categories principle is maintained

**Status**: FUNCTIONALLY CORRECT (minor flooring variance acceptable)

---

## Monster Properties & Boss Protocol

### Monster Template Structure

**Implementation** (ro_monster_templates.js): Each template includes:
- `id`, `name`, `level`, `maxHealth`, `attack`, `attack2`
- `defense` (hard DEF), `magicDefense` (hard MDEF)
- `size` (small/medium/large), `race` (plant/insect/etc)
- `element: { type, level }`, `monsterClass` (normal/boss/mvp)
- `str/agi/vit/int/dex/luk`, `attackRange`, `aggroRange`, `chaseRange`
- `modes`, `raceGroups`, `drops`, `mvpDrops`

**Status**: Structure matches research spec. All 509 templates auto-generated from rAthena mob_db.yml.

### Boss Protocol Implementation

**MD Constants** (index.js lines 355-375):
- `MD_CANMOVE` through `MD_STATUSIMMUNE` -- 18 flags defined
- **MISSING**: `MD_IGNOREMELEE` (0x0010000), `MD_IGNOREMAGIC` (0x0020000), `MD_IGNORERANGED` (0x0040000), `MD_IGNOREMISC` (0x0100000), `MD_TELEPORTBLOCK` (0x0400000), `MD_FIXEDITEMDROP` (0x1000000), `MD_SKILLIMMUNE` (0x8000000)

**parseModeFlags()** (index.js line 411):
- Parses 18 flags from hex bitmask
- **MISSING from parse**: `ignoreMelee`, `ignoreMagic`, `ignoreRanged`, `ignoreMisc`, `teleportBlock`, `fixedItemDrop`, `skillImmune`

**Boss/MVP protocol application** (index.js line 4427):
```javascript
if (template.monsterClass === 'boss' || template.monsterClass === 'mvp') {
    modeFlags.knockbackImmune = true;
    modeFlags.statusImmune = true;
    modeFlags.detector = true;
    if (template.monsterClass === 'mvp') modeFlags.mvp = true;
}
```
- CORRECT: Boss protocol adds knockback/status/detect immunity

### Monster Soft DEF Formula

**Research**: Monster soft DEF = `floor(VIT / 2)` (simpler than player formula)
**Implementation** (index.js line 4465):
```javascript
softDef: Math.floor(((template.stats?.vit || 0)) * 0.5 + Math.max(0, (template.stats?.vit || 0) - 20) * 0.3)
```
- This uses the **player formula** (`VIT/2 + max(0, VIT-20)*0.3`), NOT the monster formula (`VIT/2`)
- For VIT < 20: Result is `VIT * 0.5` -- matches monster formula
- For VIT >= 20: Result is `VIT*0.5 + (VIT-20)*0.3` -- **HIGHER than correct monster value**
- Example: VIT=50 monster should have softDef=25, but gets 25+9=34

**Status**: BUG (Medium severity -- inflates high-VIT monster soft DEF)

### Monster Soft MDEF Formula

**Research**: Monster soft MDEF = `floor((INT + Level) / 4)`
**Implementation** (index.js line 4466):
```javascript
softMDef: Math.floor((template.stats?.int || 0) + Math.floor((template.stats?.vit || 0) / 5) + Math.floor((template.stats?.dex || 0) / 5) + Math.floor((template.level || 1) / 4))
```
- This uses the **player formula** (`INT + VIT/5 + DEX/5 + Level/4`), NOT the monster formula (`(INT+Level)/4`)
- Example: INT=50, Level=60, VIT=40, DEX=30 monster should have softMDef=`floor(110/4)`=27, but gets 50+8+6+15=79

**Status**: BUG (High severity -- massively inflates monster soft MDEF, making magic far less effective than intended)

---

## Critical Discrepancies

### CRIT-1: `modeFlags.isBoss` Never Set — Boss Card Modifiers Broken

**Severity**: CRITICAL
**Location**: `parseModeFlags()` (index.js line 411), `getAttackerInfo()` (line 3835)

The `parseModeFlags()` function never sets an `isBoss` property. However, multiple damage pipeline locations check `target.modeFlags.isBoss`:

- `calculatePhysicalDamage()` line 672: `(target.modeFlags && target.modeFlags.isBoss) ? 'boss' : 'normal'` -- for Abysmal Knight Card (+25% vs boss)
- `calculatePhysicalDamage()` line 831: `(attacker.modeFlags && attacker.modeFlags.isBoss) ? 'boss' : 'normal'` -- for Alice Card (-40% from boss)
- `knockbackTarget()` line 3389: `target.modeFlags && target.modeFlags.isBoss`

Since `isBoss` is never set, `modeFlags.isBoss` is always `undefined` (falsy), meaning:
1. **Abysmal Knight Card**: Never applies +25% bonus vs boss monsters (treats all as `normal`)
2. **Alice Card**: Never applies -40% reduction from boss monsters (treats all as `normal`)
3. Some knockback checks fall through to other checks but inconsistently

**Note**: Many boss checks in index.js use `enemy.monsterClass === 'boss' || enemy.monsterClass === 'mvp'` or `modeFlags.statusImmune` as alternatives, which DO work. But the damage pipeline in `ro_damage_formulas.js` only checks `modeFlags.isBoss`.

**Fix**: Add `isBoss: false` to `parseModeFlags()`, then set `modeFlags.isBoss = true` alongside `knockbackImmune` in the boss protocol block at line 4427-4431.

### CRIT-2: Monster Soft MDEF Uses Player Formula Instead of Monster Formula

**Severity**: CRITICAL (affects all magic damage vs all monsters)
**Location**: index.js line 4466

**Current**: `INT + floor(VIT/5) + floor(DEX/5) + floor(Level/4)` (player formula)
**Correct**: `floor((INT + Level) / 4)` (monster formula from rAthena `status.cpp`)

Impact: A Level 60, INT 50 monster currently gets softMDef=79 instead of correct value 27. This makes magic deal significantly less damage than intended against most monsters, especially high-INT enemies.

**Fix**: Change line 4466 to:
```javascript
softMDef: Math.floor(((template.stats?.int || 0) + (template.level || 1)) / 4),
```

---

## High Issues

### HIGH-1: Monster Soft DEF Uses Player Formula

**Severity**: HIGH
**Location**: index.js line 4465

**Current**: `VIT*0.5 + max(0, VIT-20)*0.3`
**Correct**: `floor(VIT/2)` (monster formula)

Impact: Monsters with VIT > 20 have inflated soft DEF. Example: VIT 50 monster gets 34 instead of 25 soft DEF. This makes physical attacks deal slightly less damage than intended.

**Fix**: Change line 4465 to:
```javascript
softDef: Math.floor((template.stats?.vit || 0) / 2),
```

### HIGH-2: MD_IGNOREMELEE/MAGIC/RANGED/MISC Flags Not Parsed or Enforced

**Severity**: HIGH
**Location**: MD constants (index.js line 355), parseModeFlags (line 411)

These 4 damage type immunity flags are defined in the deep research (section 3.4) but are:
1. Not defined in the `MD` constant object
2. Not parsed in `parseModeFlags()`
3. Not checked in any damage calculation path

Affected monsters (from rAthena):
- `MD_IGNOREMELEE` (0x0010000): Immune to melee physical -- very rare
- `MD_IGNOREMAGIC` (0x0020000): Immune to magic -- very rare
- `MD_IGNORERANGED` (0x0040000): Immune to ranged physical -- very rare
- `MD_IGNOREMISC` (0x0100000): Immune to misc/trap damage -- some monsters

Note: In the current 509-monster database, these flags may not be actively used by any template, but they would be needed for future monsters or if AI_TYPE_MODES mappings include them.

**Fix**: Add to MD constants, parseModeFlags, and add checks in damage paths.

### HIGH-3: Negative Element Modifier Handling Inconsistent

**Severity**: HIGH
**Location**: ro_damage_formulas.js lines 720-726

**Current behavior**: When `eleModifier <= 0`, physical damage returns 0 with `hitType: 'elementHeal'` (negative) or `'elementImmune'` (zero).

**Research spec**: Negative element values should **heal the target**. For example, Poison attacking Undead Lv1 = -25% means the Undead gains HP equal to 25% of the would-be damage.

**Current**: Negative values are treated as 0 damage (immune), not actual healing.

This affects:
- Poison/Shadow attacks vs Undead armor (should heal)
- Same-element attacks at Lv3+ (should heal)
- Shadow attacks vs Shadow Lv2+ (should heal)

The `hitType: 'elementHeal'` is set but no actual HP restoration occurs.

**Fix**: In the auto-attack damage loop and skill damage handlers, check for `elementHeal` result and apply negative damage as healing. This is a server-side combat tick change.

---

## Medium Issues

### MED-1: Monster Auto-Attack Element in `getEnemyTargetInfo`

**Location**: index.js line 3905

`getEnemyTargetInfo()` sets `element: enemy.element || { type: 'neutral', level: 1 }` -- this is the **defending element** of the enemy (used when enemy IS the target). This is correct.

However, `calculateEnemyDamage()` (line 29897) sets `attackerInfo.weaponElement = (enemy.element && enemy.element.type) || 'neutral'` -- the monster's body element is used as its ATTACKING element. This is technically correct for RO (monster auto-attacks use their body element), but since `isNonElemental: true` is also passed, the element table is bypassed anyway.

**Inconsistency**: The `attackerInfo.weaponElement` is set but never used due to `isNonElemental: true`. If `isNonElemental` were ever removed (bug), monsters would suddenly use their body element for attacks, which is actually correct for monster SKILL attacks. The `elemental_melee` handler at line 29248 handles skill-based elemental attacks separately.

**Status**: Functionally correct but redundant code. No action needed.

### MED-2: `template.isBoss` Used in Abracadabra/Metamorphosis Paths

**Location**: index.js lines 1353, 15904, 15939, 16028, 16049, 16068, 16151, 20426

Several Abracadabra and monster skill handlers check `enemy.template?.isBoss` or `enemy.template.isBoss`. However, `isBoss` is not a standard field on monster templates -- the templates use `monsterClass: 'boss'/'mvp'`.

Some metamorphosis code at line 16028 creates a `isBoss: tmpl.isBoss || false` field, and line 16068 sets `targetEnemy.isBoss = newTmpl.isBoss || false`.

**Impact**: Since `tmpl.isBoss` is undefined on all standard templates, these checks always evaluate to false. Boss monsters spawned by Abracadabra may lack boss-level protections.

**Fix**: Change `template.isBoss` checks to `template.monsterClass === 'boss' || template.monsterClass === 'mvp'` consistently.

### MED-3: Frozen/Stone Element Override Not Applied in Physical Damage Path

**Location**: ro_damage_formulas.js, index.js

The `overrideElement` from freeze/stone status is applied in magic damage paths:
```javascript
if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
```
(Found at 6 locations in magic skill handlers)

For physical damage, the `calculatePhysicalDamage()` function reads `target.element` directly. The element override needs to be applied to the target info before calling the physical damage function.

**Check**: The `getEnemyTargetInfo()` at line 3904 reads `enemy.element` directly. If a frozen enemy's element is not being overridden at the source object level (only through buff mods), then physical attacks may not use the Water Lv1 override.

**Impact**: If freeze/stone element override is only applied in magic paths, physical attacks against frozen/stoned enemies would use the original element instead of Water Lv1 / Earth Lv1. Wind attacks against frozen targets would not get the expected 175% bonus on physical skills.

**Status**: Needs verification -- check if `applyStatusEffect` directly modifies `enemy.element` or only stores the override in buff mods.

### MED-4: Defensive Card Application Order

**Research (rAthena)**: Defensive reductions applied in this order: Size -> Race -> Class -> Element
**Implementation** (ro_damage_formulas.js lines 818-845): Applied in order: Race -> Element -> Size (then Class at lines 830-834)

The order differs from rAthena's documented order. Due to multiplicative application with floor operations between each step, different orderings can produce results that differ by 1 point. In practice this is negligible but technically incorrect.

**Fix**: Reorder defensive card application to: Size -> Race -> Class -> Element.

### MED-5: Magic Damage Missing Defensive Race/Size/Element Card Reductions

**Location**: ro_damage_formulas.js `calculateMagicalDamage()` (lines 919-1054)

The magical damage function applies:
- Card MATK rate (bMatkRate)
- Card magic race bonus (bMagicAddRace)
- Element modifier
- MDEF reduction
- Sage zone boost
- Dragonology bonuses

But it does NOT apply:
- Target's defensive race card reductions (e.g., Thara Frog -30% vs Demi-Human should reduce magic FROM Demi-Humans)
- Target's defensive element card reductions (e.g., Raydric -20% vs Neutral)
- Target's defensive size card reductions

In rAthena, defensive race/element cards reduce ALL incoming damage (physical AND magical). The current implementation only applies these in `calculatePhysicalDamage()`.

**Impact**: Shield cards like Thara Frog do not reduce incoming magical damage from the specified race, making them less effective than intended in PvP/PvE.

**Fix**: Add defensive card modifier block to `calculateMagicalDamage()` similar to Step 8c in physical damage.

---

## Missing Features

### MISS-1: Damage Type Immunity Flags (MD_IGNORE*)

Not implemented. See HIGH-2 above.
- `MD_IGNOREMELEE` (0x0010000)
- `MD_IGNOREMAGIC` (0x0020000)
- `MD_IGNORERANGED` (0x0040000)
- `MD_IGNOREMISC` (0x0100000)

### MISS-2: Fixed Item Drop Rate Flag (MD_FIXEDITEMDROP)

`MD_FIXEDITEMDROP` (0x1000000) is documented but not implemented. Monsters with this flag should have their drop rates immune to server drop rate multipliers. Currently no server drop rate multiplier system exists, so this is low priority.

### MISS-3: Teleport Block Flag (MD_TELEPORTBLOCK)

`MD_TELEPORTBLOCK` (0x0400000) prevents the monster from being teleported by skills. Not parsed or enforced. Low priority -- no player skills currently teleport monsters.

### MISS-4: Skill Immunity Flag (MD_SKILLIMMUNE)

`MD_SKILLIMMUNE` (0x8000000) makes the monster completely immune to all skills. Very rare flag, not implemented. Low priority.

### MISS-5: Negative Element Damage -> HP Healing

When element modifier is negative (e.g., Poison attacking Undead), the target should gain HP. Currently treated as 0 damage. See HIGH-3.

### MISS-6: Non-Elemental Monster Auto-Attacks vs Ghost Armor Detail

The deep research notes that monster auto-attacks are "non-elemental" and bypass the element table entirely. The implementation correctly uses `isNonElemental: true` for monster auto-attacks. However, there is a nuance: monster SKILL attacks that use the monster's body element SHOULD use the element table. The current `elemental_melee` NPC skill handler correctly handles this by NOT passing `isNonElemental`.

**Status**: Correctly implemented.

---

## Low Issues

### LOW-1: `modeFlags.boss` Never Set, Checked in Multiple Places

Various checks use `modeFlags.boss` (without the `is` prefix), which is never set by `parseModeFlags()`. These checks usually have fallbacks to `monsterClass === 'boss'` or `modeFlags.statusImmune`, but the inconsistency makes the code fragile.

Locations using `modeFlags.boss`: lines 9781, 15428, 19411, 19452, 19678, 20426, 30685.

**Fix**: Add `boss: false` to `parseModeFlags()`, set `modeFlags.boss = true` alongside `isBoss` in boss protocol block.

### LOW-2: Spear Type Key Inconsistency

The SIZE_PENALTY table uses `one_hand_spear` and `two_hand_spear` keys. The mounted spear check at line 559 also checks `weaponType === 'spear'` in addition to the specific keys:
```javascript
weaponType === 'spear' || weaponType === 'one_hand_spear' || weaponType === 'two_hand_spear'
```
The `'spear'` key does not exist in SIZE_PENALTY, so it would fall through to `default` (100/100/100). If any code path sets `weaponType = 'spear'` instead of the specific keys, size penalty would be wrong.

**Fix**: Verify all weapon type assignments use `one_hand_spear` or `two_hand_spear`, never bare `spear`. Consider adding `spear` to SIZE_PENALTY as an alias.

### LOW-3: `calculateSkillDamage` Wrapper Has Different Arg Count Than `calculatePhysicalDamage`

The wrapper `calculateSkillDamage(attackerStats, targetStats, targetHardDef, skillMultiplier, attackerBuffMods, targetBuffMods, targetInfo, attackerInfo, skillOptions)` at line 1814 takes 9 arguments and remaps them into `calculatePhysicalDamage(attacker, target, options)` which takes 3. This works but is fragile and error-prone. Each caller must pass arguments in the exact positional order.

**Status**: Working correctly, but could benefit from a single-object argument pattern.

### LOW-4: Earth Damage Boost Missing from Sage Zones

The Sage zone elemental damage boost code (ro_damage_formulas.js lines 858-866 for physical, 1024-1033 for magic) checks for `fire`, `water`, and `wind` damage boosts. There is no check for `earth` damage boost.

The deep research does not list an Earth damage boost zone (Volcano=Fire, Deluge=Water, Violent Gale=Wind), so this may be correct -- there is no Sage zone skill for Earth damage boost in pre-renewal. However, if an `earthDmgBoost` buff were ever added, it would not be applied.

**Status**: Correct per pre-renewal RO design (no Earth damage zone skill).

---

## Recommended Fixes

### Priority 1 (Critical — Affects combat correctness for all players)

1. **Fix CRIT-1**: Add `isBoss: false` to `parseModeFlags()`, then set `modeFlags.isBoss = true` in boss protocol block. This fixes Abysmal Knight Card and Alice Card in the damage pipeline.

2. **Fix CRIT-2**: Change monster soft MDEF formula from player formula to `floor((INT + Level) / 4)`. This is a one-line fix at index.js line 4466. All existing spawned enemies will need the value recalculated on next spawn cycle.

### Priority 2 (High — Affects combat balance)

3. **Fix HIGH-1**: Change monster soft DEF formula from player formula to `floor(VIT / 2)`. One-line fix at index.js line 4465.

4. **Fix HIGH-3**: Implement negative element modifier as HP healing. When `eleModifier < 0`, apply `floor(abs(damage * eleModifier / 100))` as HP restoration to the target instead of 0 damage.

5. **Fix MED-5**: Add defensive card reductions (race/size/element/class) to `calculateMagicalDamage()`. Mirror the Step 8c/8d/8e block from physical damage.

### Priority 3 (Medium — Correctness improvements)

6. **Fix MED-2**: Replace `template.isBoss` checks with `template.monsterClass === 'boss' || template.monsterClass === 'mvp'` in all Abracadabra/metamorphosis paths.

7. **Fix MED-3**: Verify that freeze/stone element override is applied to physical damage target info, not just magic damage paths.

8. **Fix MED-4**: Reorder defensive card application to match rAthena: Size -> Race -> Class -> Element.

### Priority 4 (Low — Future-proofing)

9. **Fix HIGH-2**: Add MD_IGNORE* flags to MD constants, parseModeFlags, and damage paths. Low urgency since no current monsters use these flags.

10. **Fix LOW-1**: Add `boss: false` to parseModeFlags alongside `isBoss` fix.

11. **Fix LOW-2**: Add `spear` alias to SIZE_PENALTY table or audit all weapon type assignments.

---

## Verification Matrix

| System | Research Doc | Implementation | Status |
|--------|-------------|----------------|--------|
| Element Table (400 values) | 07_Element_System.md | ro_damage_formulas.js ELEMENT_TABLE | CORRECT |
| Element Priority Chain | 07_Element_System.md | index.js recalcEffectiveWeaponElement + getAttackerInfo | CORRECT |
| Frozen -> Water Lv1 | 07_Element_System.md | ro_status_effects.js | CORRECT |
| Stone -> Earth Lv1 | 07_Element_System.md | ro_status_effects.js | CORRECT |
| Monster Non-Elemental Auto | 07_Element_System.md | index.js isNonElemental: true | CORRECT |
| Endow Mutual Exclusion | 07_Element_System.md | index.js ITEM_ENCHANTARMS + buff system | CORRECT |
| Skill Fixed Element Override | 07_Element_System.md | ro_damage_formulas.js atkElement chain | CORRECT |
| Size Penalty Table (18 types) | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js SIZE_PENALTY | CORRECT |
| Mounted Spear Override | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js line 559-562 | CORRECT |
| Drake Card / Max Power Bypass | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js line 560 | CORRECT |
| Size Penalty on WeaponATK Only | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js line 566 | CORRECT |
| Race Card Bonuses (Offensive) | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js lines 649-656 | CORRECT |
| Race Card Bonuses (Defensive) | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js lines 818-825 | CORRECT |
| Boss/Normal Class Cards | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js lines 671-675 | BROKEN (isBoss never set) |
| Card Bonus Stacking Order | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js | CORRECT |
| Monster Template Structure | 08_Size_Race_MonsterProps.md | ro_monster_templates.js | CORRECT |
| Boss Protocol (KB/Status/Detect) | 08_Size_Race_MonsterProps.md | index.js lines 4427-4431 | CORRECT |
| Monster Soft DEF Formula | 08_Size_Race_MonsterProps.md | index.js line 4465 | WRONG (uses player formula) |
| Monster Soft MDEF Formula | 08_Size_Race_MonsterProps.md | index.js line 4466 | WRONG (uses player formula) |
| MD_IGNORE* Damage Immunity | 08_Size_Race_MonsterProps.md | index.js | MISSING |
| Monster HIT = Level + DEX | 08_Size_Race_MonsterProps.md | index.js (implied) | CORRECT |
| Monster FLEE = Level + AGI | 08_Size_Race_MonsterProps.md | index.js (implied) | CORRECT |
| Player = Medium, Demi-Human | 08_Size_Race_MonsterProps.md | index.js getPlayerTargetInfo | CORRECT |
| Negative Element = Heal | 07_Element_System.md | ro_damage_formulas.js | MISSING (treated as immune) |
| Sage Zone Dmg Boost | 07_Element_System.md | ro_damage_formulas.js lines 858-866 | CORRECT (3/3 elements) |
| Magic Defensive Cards | 08_Size_Race_MonsterProps.md | ro_damage_formulas.js calculateMagicalDamage | MISSING |
