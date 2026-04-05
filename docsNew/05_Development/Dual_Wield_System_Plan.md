# Dual Wield System — Full Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Combat_System](../03_Server_Side/Combat_System.md) | [Assassin_Class_Research](Assassin_Class_Research.md)

**Status:** IMPLEMENTED (Phases A-H Complete)
**Date:** 2026-03-12
**RO Classic Reference:** Pre-Renewal Assassin Dual Wielding
**Systems Touched:** Server (index.js, ro_damage_formulas.js, ro_exp_tables.js, ro_skill_data.js), Client (EquipmentSubsystem, SEquipmentWidget, CharacterData.h, InventorySubsystem, combat events), Database (character_inventory)

---

## Table of Contents

1. [RO Classic Dual Wield Rules (Research)](#1-ro-classic-dual-wield-rules)
2. [Equipment Slot Mechanics](#2-equipment-slot-mechanics)
3. [Righthand & Lefthand Mastery Skills](#3-righthand--lefthand-mastery-skills)
4. [Damage Formulas Per Hand](#4-damage-formulas-per-hand)
5. [ASPD Calculation for Dual Wield](#5-aspd-calculation-for-dual-wield)
6. [Auto-Attack Cycle: Both Hands Per Hit](#6-auto-attack-cycle-both-hands-per-hit)
7. [Left Hand as "Skill Attack" — Special Rules](#7-left-hand-as-skill-attack--special-rules)
8. [Double Attack Interaction](#8-double-attack-interaction)
9. [Critical Hit Behavior](#9-critical-hit-behavior)
10. [Card & Element Rules](#10-card--element-rules)
11. [Skill Damage with Dual Wield](#11-skill-damage-with-dual-wield)
12. [Equipment UI Changes](#12-equipment-ui-changes)
13. [Server Implementation Tasks](#13-server-implementation-tasks)
14. [Client Implementation Tasks](#14-client-implementation-tasks)
15. [Socket.io Event Changes](#15-socketio-event-changes)
16. [Database Changes](#16-database-changes)
17. [Implementation Phases & Checklist](#17-implementation-phases--checklist)

---

## 1. RO Classic Dual Wield Rules

### 1.1 Who Can Dual Wield

Only **Assassin** and **Assassin Cross** (transcendent) can equip a weapon in each hand. No other class has this ability in pre-renewal RO.

**Sources:** [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding), [iRO Wiki Classic — Assassin](https://irowiki.org/classic/Assassin)

### 1.2 Allowed Left-Hand Weapon Types

The left hand (off-hand) can hold only these one-handed weapon types:

| Weapon Type | DB `weapon_type` | rAthena SubType |
|-------------|-----------------|-----------------|
| Dagger | `dagger` | `Dagger` |
| 1H Sword | `one_hand_sword` | `1hSword` |
| 1H Axe | `axe` | `1hAxe` |

All valid dual-wield combinations:
- 2× Daggers
- 2× 1H Swords
- 2× 1H Axes
- Dagger + 1H Sword (either hand)
- Dagger + 1H Axe (either hand)
- 1H Sword + 1H Axe (either hand)

**NOT allowed in left hand:** Maces, Staves, Knuckles, Books, Bows, or any two-handed weapon. Katars occupy BOTH hand slots (they are NOT dual-wielded — they are a single two-handed weapon type).

**Sources:** [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding), [iRO Wiki — Assassin](https://irowiki.org/wiki/Assassin), [rAthena Source](https://rathena.org/board/topic/104092-dual-wielding-for-all-jobs/)

### 1.3 Katar vs. Dual Wield

Katars and dual wield are **mutually exclusive** weapon modes for Assassin:

| Property | Katar | Dual Wield |
|----------|-------|------------|
| Hand slots used | Both (single weapon) | Both (two weapons) |
| Critical rate | Double (2× crit) | Normal (1× crit) |
| Double Attack | Only with Sidewinder Card | Natural with Dagger right hand |
| Sonic Blow | Yes | No (Katar only) |
| Grimtooth | Yes | No (Katar only) |
| Katar Mastery | Yes (+3 ATK/level) | No effect |
| Righthand/Lefthand Mastery | No effect | Yes |
| Shield equippable | No | No |
| Per-hit damage | Full (one weapon) | Penalized (two weapons) |
| Effective DPS | Fewer hits, full damage | More hits, reduced per-hit |

**Source:** [MyRoLife — Katar vs Dual-Dagger](https://myrolife.blogspot.com/2016/02/assassin-katar-vs-dual-dagger.html)

---

## 2. Equipment Slot Mechanics

### 2.1 Slot Layout for Dual Wield Classes

For Assassin/Assassin Cross, the equipment window has two weapon slots instead of weapon + shield:

| Slot Position | DB `equipped_position` | Display Name | Purpose |
|--------------|----------------------|-------------|---------|
| Right Hand | `weapon` | Right Hand | Primary weapon (main hand) |
| Left Hand | `weapon_left` | Left Hand | Off-hand weapon (dual wield only) |

For **non-Assassin classes**, `weapon_left` position is never used — the `shield` slot remains as-is.

### 2.2 Weapon Equipping Logic (Server-Side)

The equip handler must follow these rules for Assassin/Assassin Cross:

```
1. If player is NOT Assassin/Assassin Cross:
   → Standard single-weapon equip (current behavior)
   → Shield slot works normally

2. If player IS Assassin/Assassin Cross AND item is a valid dual-wield weapon type:
   a. If right hand is empty:
      → Equip to `weapon` (right hand)
   b. If right hand has weapon AND left hand is empty:
      → Equip to `weapon_left` (left hand)
   c. If BOTH hands have weapons:
      → Replace RIGHT hand weapon (unequip old right, equip new to `weapon`)
   d. If equipping a Katar:
      → Unequip BOTH hand slots, equip Katar to `weapon` (Katar uses both)
      → Block left hand equipping while Katar equipped

3. Shield interaction:
   → Assassin CAN equip shields (e.g., Guard) ONLY if left hand is empty
   → Equipping a left-hand weapon while shield is equipped:
     unequip shield first, then equip weapon to left hand
   → Equipping a shield while left-hand weapon is equipped:
     unequip left-hand weapon first, then equip shield

4. Two-handed weapon interaction:
   → Same as existing: unequip left hand weapon (if any) + shield
```

### 2.3 Unequip Logic

```
- Unequip right hand weapon:
  → If left hand has weapon, left hand stays equipped
  → Mastery penalties still apply to left hand alone? NO — if only left hand is equipped,
    it behaves as a normal single weapon (no dual wield penalties)
    [rAthena: left hand only = unarmed main hand damage, not dual wield mode]

- Unequip left hand weapon:
  → Right hand stays equipped, no more dual wield mode
  → Mastery penalties no longer apply

- Unequip Katar:
  → Both slots freed
```

### 2.4 Dual Wield Detection

A player is "dual wielding" when:
```javascript
function isDualWielding(player) {
    return player.equippedWeaponRight && player.equippedWeaponLeft;
}
```

This flag determines whether mastery penalties apply and whether the attack cycle produces two hits.

**Sources:** [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding), [iRO Wiki Classic — Assassin](https://irowiki.org/classic/Assassin)

---

## 3. Righthand & Lefthand Mastery Skills

### 3.1 Righthand Mastery (Skill ID: TBD)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | None |
| Available Classes | Assassin, Assassin Cross |
| Affects | Auto-attacks ONLY (not skills) |
| No effect on | Katars |

**Damage Multiplier Per Level (Right Hand):**

| Level | Damage % | Formula |
|-------|----------|---------|
| 0 (no skill) | 50% | Base penalty |
| 1 | 60% | 50 + 10×1 |
| 2 | 70% | 50 + 10×2 |
| 3 | 80% | 50 + 10×3 |
| 4 | 90% | 50 + 10×4 |
| 5 | 100% | 50 + 10×5 (full damage) |

**Formula:** `rightHandDamagePercent = 50 + (righHandMasteryLevel × 10)`

### 3.2 Lefthand Mastery (Skill ID: TBD)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Righthand Mastery Lv. 2 |
| Available Classes | Assassin, Assassin Cross |
| Affects | Auto-attacks ONLY (not skills) |
| No effect on | Katars |

**Damage Multiplier Per Level (Left Hand):**

| Level | Damage % | Formula |
|-------|----------|---------|
| 0 (no skill) | 30% | Base penalty |
| 1 | 40% | 30 + 10×1 |
| 2 | 50% | 30 + 10×2 |
| 3 | 60% | 30 + 10×3 |
| 4 | 70% | 30 + 10×4 |
| 5 | 80% | 30 + 10×5 (max recovery) |

**Formula:** `leftHandDamagePercent = 30 + (leftHandMasteryLevel × 10)`

**IMPORTANT:** Left hand can NEVER reach 100% damage. The maximum is 80% at Lv5. This is intentional balance — dual wield damage output should not exceed single-weapon DPS without significant investment.

### 3.3 rAthena Source Reference

From `src/map/battle.c`:
```c
// Right hand penalty
ATK_RATER(wd.damage, 50 + (skill_lv * 10))  // skill_lv = AS_RIGHT (Righthand Mastery) level

// Left hand penalty
ATK_RATEL(wd.damage2, 30 + (skill_lv * 10)) // skill_lv = AS_LEFT (Lefthand Mastery) level
```

Both have a minimum damage floor of 1.

**Sources:** [iRO Wiki — Righthand Mastery](https://irowiki.org/wiki/Righthand_Mastery), [iRO Wiki — Lefthand Mastery](https://irowiki.org/wiki/Lefthand_Mastery), [iRO Wiki Classic — Righthand Mastery](https://irowiki.org/classic/Righthand_Mastery), [rAthena Forum](https://rathena.org/board/topic/113069-left-hand-on-dual-wield-reduces-damage/)

---

## 4. Damage Formulas Per Hand

### 4.1 Per-Hand Damage Calculation

Each hand's damage is calculated **independently** through the full physical damage pipeline. The entire 16-step pipeline runs TWICE per attack cycle — once for each weapon.

```
=== RIGHT HAND (wd.damage) ===
1-16. Full physical damage pipeline using:
      - Right hand weapon ATK, element, refine, cards
      - Right hand weapon type for size penalty
      - Player's StatusATK (same for both hands)
17.   Apply mastery penalty: damage = floor(damage * rightHandPercent / 100)
      where rightHandPercent = 50 + (RighHandMasteryLv * 10)
18.   Floor to minimum 1

=== LEFT HAND (wd.damage2) ===
1-16. Full physical damage pipeline using:
      - Left hand weapon ATK, element, refine, cards
      - Left hand weapon type for size penalty
      - Player's StatusATK (same for both hands)
17.   Apply mastery penalty: damage2 = floor(damage2 * leftHandPercent / 100)
      where leftHandPercent = 30 + (LeftHandMasteryLv * 10)
18.   Floor to minimum 1
```

### 4.2 StatusATK

StatusATK is the SAME for both hands (it's stat-based, not weapon-based):
```
StatusATK = STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
```

### 4.3 WeaponATK

WeaponATK is DIFFERENT per hand (each weapon has its own ATK value):
```
Right: WeaponATK_R = random(min(DEX*(0.8+0.2*WeapLvR), ATK_R), ATK_R)
Left:  WeaponATK_L = random(min(DEX*(0.8+0.2*WeapLvL), ATK_L), ATK_L)
```

### 4.4 Size Penalty

Each hand has its own size penalty based on its weapon type:
```
Right: sizePenalty_R = getSizePenalty(rightWeaponType, targetSize)
Left:  sizePenalty_L = getSizePenalty(leftWeaponType, targetSize)
```

Skills/cards that nullify size penalty (Drake Card, Weapon Perfection) benefit BOTH hands.

### 4.5 Refine Bonus

Each hand has its own refine bonus:
```
Right: refineATK_R = getRefineAtkBonus(rightWeaponLevel, rightRefineLevel)
Left:  refineATK_L = getRefineAtkBonus(leftWeaponLevel, leftRefineLevel)
```

### 4.6 When NOT Dual Wielding

If only one weapon is equipped (no dual wield), NO mastery penalty applies. The penalties only activate when BOTH hand slots have weapons.

If only the LEFT hand has a weapon (right hand empty), the player attacks as if **unarmed** with the main hand — the left hand weapon is NOT used for normal attacks. (rAthena behavior: left-hand-only = unarmed main hand damage)

**Sources:** [iRO Wiki Classic — Attacks](https://irowiki.org/classic/Attacks), [RateMyServer Forum — Double Dagger Details](https://forum.ratemyserver.net/general-discussion/double-dagger-details/), [rAthena Forum](https://rathena.org/board/topic/113069-left-hand-on-dual-wield-reduces-damage/)

---

## 5. ASPD Calculation for Dual Wield

### 5.1 Pre-Renewal Dual Wield ASPD Formula

The standard pre-renewal ASPD formula is:
```
WD = ASPD_BASE_DELAYS[class][weaponType]
ASPD = 200 - floor((WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod))
```

For dual wielding, the Weapon Delay is calculated as:
```
WD_dual = floor((ASPD_BASE_DELAYS[class][rightWeaponType] + ASPD_BASE_DELAYS[class][leftWeaponType]) * 7 / 10)
```

Then feed `WD_dual` into the standard formula in place of `WD`.

### 5.2 Our Existing ASPD Base Delays (from `ro_exp_tables.js`)

For Assassin class (also used by Assassin Cross via `TRANS_TO_BASE_CLASS`):

| Weapon Type | Base Delay |
|-------------|-----------|
| bare_hand | 38 |
| dagger | 45 |
| one_hand_sword | 65 |
| katar | 42 |

**NOTE:** We need to add `axe` for dual wield with axes. Checking rAthena's Assassin ASPD for axes — Assassins CAN equip axes per the weapon type rules but we don't have the value. From rAthena data, the Assassin axe delay should be approximately 75-80 (similar to Merchant's 70). We'll use **75** as a reasonable value or verify against rAthena's `job_db.yml`.

### 5.3 Dual Wield ASPD Examples

**2× Daggers (Assassin, 99 AGI, 1 DEX, no mods):**
```
WD = floor((45 + 45) * 7 / 10) = floor(63) = 63
inner = floor((63*99/25 + 63*1/100) / 10) = floor((249.48 + 0.63) / 10) = floor(25.01) = 25
ASPD = 200 - floor((63 - 25) * 1) = 200 - 38 = 162
```

**Dagger + 1H Sword (Assassin, 99 AGI, 1 DEX):**
```
WD = floor((45 + 65) * 7 / 10) = floor(77) = 77
inner = floor((77*99/25 + 77*1/100) / 10) = floor((304.92 + 0.77) / 10) = floor(30.57) = 30
ASPD = 200 - floor((77 - 30) * 1) = 200 - 47 = 153
```

**Single Dagger (no dual wield, 99 AGI, 1 DEX):**
```
WD = 45
inner = floor((45*99/25 + 45*1/100) / 10) = floor((178.2 + 0.45) / 10) = floor(17.86) = 17
ASPD = 200 - floor((45 - 17) * 1) = 200 - 28 = 172
```

### 5.4 Hits Per Second with Dual Wield

The hits-per-second formula:
```
HitsPerSecond = 50 / (200 - ASPD)
```

But when dual wielding, each "hit" is actually TWO hits (one per hand). So effective hits per second:
```
EffectiveHitsPerSecond = 2 * (50 / (200 - ASPD))
```

| Setup | ASPD | HPS (cycles) | HPS (actual hits) |
|-------|------|-------------|-------------------|
| 2× Daggers (99 AGI) | 162 | 1.32 | 2.63 |
| Dagger+Sword (99 AGI) | 153 | 1.06 | 2.13 |
| Single Dagger (99 AGI) | 172 | 1.79 | 1.79 |
| Katar (99 AGI) | ~165 | 1.43 | 1.43 |

### 5.5 ASPD Cap

ASPD is capped at **190** (same as all other classes). With dual wield, this is harder to reach due to the combined weapon delay being higher.

**Sources:** [iRO Wiki Classic — ASPD](https://irowiki.org/classic/ASPD), [RagnaPlace — ASPD](https://ragnaplace.com/en/wiki/irowiki-classic/ASPD), [rAthena Source](https://rathena.org/board/topic/110695-how-to-change-base-aspd/)

---

## 6. Auto-Attack Cycle: Both Hands Per Hit

### 6.1 How Both Hands Attack

When a dual-wielding Assassin auto-attacks, **BOTH weapons hit in the same attack cycle** — this is NOT alternating hands. Each attack action produces TWO damage results:

```
Attack Cycle:
  1. Check ASPD timer (using dual wield combined delay)
  2. Timer elapsed → Run damage pipeline for RIGHT hand → wd.damage
  3. Run damage pipeline for LEFT hand → wd.damage2
  4. Send BOTH damage values in ONE event to zone
  5. Client displays TWO damage numbers (one per hand)
  6. Reset ASPD timer
```

### 6.2 Attack Animation

In RO, the attack animation plays ONCE per cycle. Both damage numbers appear near-simultaneously. The client shows:
- First number: right hand damage (can be "MISS", critical, or normal)
- Second number: left hand damage (never shows "MISS", uses "skill" display style)

### 6.3 Implementation in Our Combat Tick

The `processPlayerAutoAttack()` function needs modification to handle the dual hit:

```javascript
// Pseudocode for dual wield auto-attack
if (isDualWielding(player)) {
    // Calculate right hand damage
    const rightResult = roPhysicalDamage(rightAttackerInfo, targetInfo, { isSkill: false });
    // Apply mastery penalty
    rightResult.damage = Math.max(1, Math.floor(rightResult.damage * rightHandPercent / 100));

    // Calculate left hand damage (treated as skill attack — always hits)
    const leftResult = roPhysicalDamage(leftAttackerInfo, targetInfo, { isSkill: true, forceHit: true });
    // Apply mastery penalty
    leftResult.damage = Math.max(1, Math.floor(leftResult.damage * leftHandPercent / 100));

    // Broadcast BOTH damages
    broadcastDualWieldDamage(attacker, target, rightResult, leftResult);
} else {
    // Existing single-weapon auto-attack
}
```

**Sources:** [iRO Wiki Classic — Attacks](https://irowiki.org/classic/Attacks), [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding)

---

## 7. Left Hand as "Skill Attack" — Special Rules

### 7.1 What "Skill Attack" Means

In rAthena, the left-hand hit is internally flagged as a **skill-type attack**. This has several consequences:

| Property | Right Hand | Left Hand |
|----------|-----------|-----------|
| Attack type | Normal auto-attack | "Skill" attack |
| Can miss (FLEE)? | Yes (5-95% hit rate) | **No** — always hits |
| Shows "MISS!" on client? | Yes, when miss | **Never** |
| Can critically hit? | Yes | **No** (independently) |
| Shows crit bubble? | Yes, when crit | Only if right hand crits (cosmetic) |
| Affected by FLEE? | Yes | **No** |
| Affected by Perfect Dodge? | Yes | Unclear — likely **No** (skill attacks bypass PD) |
| Double Attack proc? | Yes (if dagger) | **No** |

### 7.2 Why Left Hand Always Hits

The left-hand attack uses `forceHit: true` (or equivalent skill-attack hit logic), which bypasses the HIT vs FLEE accuracy check. This is a significant advantage of dual wielding — the off-hand damage is guaranteed.

### 7.3 Left Hand Damage Display

When the left hand "misses" (would have missed based on HIT/FLEE if checked), the game simply **doesn't display the miss** — it still deals 0 or minimum damage. In practice, since the left hand always hits, it always deals at least 1 damage.

**Sources:** [iRO Wiki Classic — Attacks](https://irowiki.org/classic/Attacks), [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding)

---

## 8. Double Attack Interaction

### 8.1 Double Attack with Dual Wield

Double Attack is a Thief passive skill that gives a chance for an auto-attack to deal double hits. When dual wielding:

- Double Attack procs **ONLY on the right-hand weapon**
- It only works when the **right-hand weapon is a Dagger**
- When it procs, the right hand deals **2 hits** instead of 1
- The left hand still deals its normal **1 hit**
- Total hits in a Double Attack cycle: **3 hits** (2 right + 1 left)

### 8.2 Double Attack Chance

Pre-renewal: `5% × skill level` (max Lv10 = 50% chance)

### 8.3 Attack Cycle with Double Attack

```
Normal dual wield cycle:    [Right: 1 hit] + [Left: 1 hit] = 2 hits
Double Attack proc:         [Right: 2 hits] + [Left: 1 hit] = 3 hits
```

When Double Attack procs:
- Right hand damage is dealt TWICE (same damage value, displayed as one bundled number)
- Left hand damage is dealt once as usual

### 8.4 Double Attack Does NOT Trigger on Left Hand

Even if the left-hand weapon is also a dagger, Double Attack NEVER procs on it. This is because the left hand is treated as a "skill attack", and Double Attack only procs on non-skill auto-attacks.

### 8.5 Our Existing Double Attack Code

We already have Double Attack implemented in the auto-attack tick (checks `doubleAttackChance` from passives, 200ms delay between hits). This needs to be extended to:
1. Only trigger on right-hand weapon when dual wielding
2. The left-hand hit happens alongside/after the Double Attack right-hand hits

**Sources:** [iRO Wiki — Double Attack](https://irowiki.org/wiki/Double_Attack), [RateMyServer — Double Attack](https://ratemyserver.net/index.php?page=skill_db&skid=48), [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding)

---

## 9. Critical Hit Behavior

### 9.1 Criticals with Dual Wield

- Only the **right hand** can critically hit
- The left hand **NEVER independently crits** (it's a "skill attack")
- If the right hand crits:
  - Right hand gets the full 40% crit damage bonus
  - The **crit visual bubble appears on BOTH hits** (cosmetic only)
  - Left hand damage is calculated **normally** (no crit bonus) — it just LOOKS like a crit
- Critical hits on right hand still bypass FLEE and Perfect Dodge (as normal)

### 9.2 Crit Rate with Dual Wield vs Katar

| Weapon Mode | Crit Rate Formula |
|-------------|------------------|
| Dual Wield | Normal: `1 + floor(LUK * 0.3) + bonuses` |
| Katar | **Double**: `2 × (1 + floor(LUK * 0.3) + bonuses)` |

This is why Katar-crit builds are popular — katars get 2× crit rate while dual wield does not.

### 9.3 Implementation

```javascript
// In dual wield damage calculation:
// Right hand: normal crit check
const rightCritCheck = !isDualWieldLeftHand; // true for right hand
// Left hand: skip crit check entirely
const leftCritCheck = false; // never crit

// Visual: if right hand crits, send isCritical: true for BOTH hits
// but only apply crit damage multiplier to right hand
```

**Sources:** [iRO Wiki Classic — Attacks](https://irowiki.org/classic/Attacks), [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding)

---

## 10. Card & Element Rules

### 10.1 Auto-Attack Card Rules

For **auto-attacks**, card effects work per-weapon:

| Card Type | Right Hand | Left Hand | Notes |
|-----------|-----------|-----------|-------|
| ATK bonus cards (Andre) | Applies to right | Applies to left | Each hand uses its own weapon's cards |
| Race cards (Hydra) | Applies to right | Applies to left | Stacks across both weapons |
| Element cards (Vadon) | Applies to right | Applies to left | Per-weapon |
| Size cards (Minorous) | Applies to right | Applies to left | Per-weapon |
| Status cards (Marina) | Both hands can proc | Both hands can proc | Status applies from either |

**Key insight:** For auto-attacks, each hand's damage pipeline uses the cards equipped IN THAT WEAPON. Cards from weapon A do not affect weapon B's damage calculation. However, total card modifiers (racial, elemental, size) stack multiplicatively across both weapons' cards for the SAME hand's calculation.

### 10.2 Auto-Attack Element Rules

Each hand uses its **own weapon's element** independently:

```
Right hand hit: uses rightWeapon.element (or endow buff element)
Left hand hit:  uses leftWeapon.element (or endow buff element)
```

This means you can have a Fire weapon in the right hand and a Holy weapon in the left hand — they hit with different elements against the same target.

**Endow skills** (Sage buffs like Endow Blaze) apply to **BOTH weapons**.

### 10.3 Skill Card & Element Rules

For **skills** (not auto-attacks), the rules are different:

| Property | Source |
|----------|--------|
| Base ATK | Right hand weapon ONLY |
| Refine bonus | Right hand weapon ONLY |
| Weapon element | Right hand weapon ONLY |
| Card effects (stat bonuses) | BOTH weapons |
| Card effects (racial/elemental/size) | BOTH weapons |
| Righthand/Lefthand Mastery | Does NOT apply (skills bypass mastery) |

So skills benefit from card bonuses in both weapons but only use the right hand's ATK and element.

### 10.4 Drake Card (Size Penalty Nullification)

Drake Card nullifies size penalties. When equipped in EITHER weapon while dual wielding, it benefits **BOTH hands** during auto-attacks. (Weapon Perfection skill has the same effect.)

### 10.5 Implementation Notes

For our system, since we calculate card bonuses via `rebuildCardBonuses()` into `player.cardMods`:
- We need to track card bonuses **per weapon** for auto-attacks
- OR: keep the current merged `cardMods` for skills, and calculate per-weapon card mods during the auto-attack pipeline

Recommended approach: Add `player.cardModsRight` and `player.cardModsLeft` alongside the existing `player.cardMods` (which remains for skills).

**Sources:** [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding), [RateMyServer Forum — Double Dagger Details](https://forum.ratemyserver.net/general-discussion/double-dagger-details/), [RagnaPlace — Dual Wielding](https://ragnaplace.com/en/wiki/irowiki/Dual_Wielding)

---

## 11. Skill Damage with Dual Wield

### 11.1 Assassin Skills and Weapon Compatibility

| Skill | Works with Dual Wield? | Uses Which Hand? | Notes |
|-------|----------------------|-----------------|-------|
| Sonic Blow | **NO** | — | Katar ONLY |
| Grimtooth | **NO** | — | Katar ONLY |
| Meteor Assault | Yes | Right hand ATK only | Left hand cards still apply |
| Venom Splasher | Yes | Right hand ATK only | |
| Soul Destroyer | Yes | Right hand ATK + MATK | |
| Envenom | Yes | Right hand ATK only | Thief skill |
| Poison React | Yes | Right hand | |
| Cloaking | Yes | N/A (no damage) | |
| Enchant Poison | Yes | Applies to right hand element | |
| Double Attack | Right hand only | Auto-attack passive | See Section 8 |

### 11.2 Key Rule: Skills NEVER Trigger Dual Wield Hits

When using a skill while dual wielding:
- The skill uses the **right hand weapon only** for base damage
- Only **ONE hit** is dealt (the skill hit) — no left-hand bonus hit
- Righthand/Lefthand Mastery penalties do NOT apply
- Card effects from BOTH weapons contribute to the skill damage

### 11.3 MATK with Dual Wield

MATK from both weapons (base + refine) stacks and affects magic skills:
```
TotalWeaponMATK = rightWeapon.matk + leftWeapon.matk + rightRefine.matkBonus + leftRefine.matkBonus
```

**Sources:** [iRO Wiki — Dual Wielding](https://irowiki.org/wiki/Dual_Wielding), [iRO Wiki — Sonic Blow](https://irowiki.org/wiki/Sonic_Blow), [RagnaPlace — Dual Wielding](https://ragnaplace.com/en/wiki/irowiki/Dual_Wielding)

---

## 12. Equipment UI Changes

### 12.1 Equipment Window Layout

The current `SEquipmentWidget` has a fixed layout with a `weapon` slot and a `shield` slot. For Assassin/Assassin Cross, the shield slot must **dynamically change** to show as a "Left Hand" weapon slot.

**Option A (Recommended):** The shield slot always exists. When an Assassin equips a weapon in the left hand, the shield slot visually transforms to show the weapon. The slot position `weapon_left` is used instead of `shield`.

**Option B:** Add a completely separate slot. More complex, less aligned with RO's design.

### 12.2 Slot Display Logic

```
If player class is Assassin/Assassin Cross:
  - Right column slot (currently Shield):
    Show as "Left Hand" label
    Accept weapons (dagger, 1H sword, 1H axe) OR shields
    If weapon is equipped here → equipped_position = 'weapon_left'
    If shield is equipped here → equipped_position = 'shield'
Else:
  - Right column slot:
    Show as "Shield" label (current behavior)
    Accept shields only
```

### 12.3 Drag-and-Drop Rules

- Dragging a dagger/sword/axe to the shield slot (for Assassin): equip as left-hand weapon
- Dragging a shield to the shield slot (for Assassin): equip as shield (unequips left weapon if present)
- Dragging a weapon to the weapon slot: always equips as right hand
- Dragging a Katar: equips to weapon slot, clears shield/left-hand slot

### 12.4 Tooltip Changes

When hovering over the left-hand weapon slot:
- Show "Left Hand" instead of "Shield" for Assassin
- Item tooltip should show the weapon's stats normally
- Add "(Off-hand)" indicator text if desired

### 12.5 Stat Display

The Combat Stats panel (F8) should reflect the combined stats from both weapons:
- ATK should show the combined effective ATK (or right-hand ATK, matching RO's display)
- Equipment bonuses from both weapons are already summed in `player.equipmentBonuses`

---

## 13. Server Implementation Tasks

### 13.1 Equipment System Changes (`index.js`)

- [ ] **Add `weapon_left` equipped_position support** — update `equipped_position` VARCHAR to accept 'weapon_left'
- [ ] **Modify `inventory:equip` handler** — implement the dual-wield equip logic from Section 2.2
- [ ] **Add `isDualWielding()` helper** — checks if both weapon slots are occupied
- [ ] **Add `canDualWield(jobClass)` helper** — returns true only for assassin/assassin_cross
- [ ] **Add `isValidLeftHandWeapon(weaponType)` helper** — checks dagger, one_hand_sword, axe
- [ ] **Track left-hand weapon stats** — `player.weaponATK_L`, `player.weaponType_L`, `player.weaponElement_L`, `player.weaponLevel_L`, `player.weaponRefine_L`
- [ ] **Update `getPlayerInventory` query** — include `equipped_position = 'weapon_left'` in equipment detection
- [ ] **Update equip bonus aggregation** — both weapons' stat bonuses go into `player.equipmentBonuses`
- [ ] **Update `player:equipment_changed` broadcast** — include left-hand weapon info
- [ ] **Handle Katar equip** — auto-unequip left-hand weapon when equipping Katar
- [ ] **Handle shield/left-weapon conflicts** — unequip one when equipping the other

### 13.2 Damage Formula Changes (`ro_damage_formulas.js`)

- [ ] **Add `calculateDualWieldDamage()` function** — runs pipeline twice (right + left), applies mastery penalties
- [ ] **Modify `calculatePhysicalDamage()` wrapper** — detect dual wield mode, call dual pipeline
- [ ] **Add mastery penalty application** — `floor(damage * masteryPercent / 100)` after full pipeline
- [ ] **Left hand forceHit** — skip HIT/FLEE check for left hand (always hits)
- [ ] **Left hand noCrit** — skip critical check for left hand
- [ ] **Card bonus per hand** — calculate card modifiers independently for each weapon's pipeline

### 13.3 ASPD Changes (`ro_exp_tables.js` + `index.js`)

- [ ] **Add axe delay to Assassin** — `assassin: { ..., axe: 75 }` (verify against rAthena)
- [ ] **Modify ASPD calculation in `calculateDerivedStats()`** — detect dual wield, use combined formula
- [ ] **Dual wield WD formula** — `floor((WD_right + WD_left) * 7 / 10)`
- [ ] **Update `getAttackIntervalMs()`** — no change needed (it already converts ASPD to ms)

### 13.4 Combat Tick Changes (`index.js`)

- [ ] **Modify `processPlayerAutoAttack()`** — add dual wield branch
- [ ] **Generate two damage results per cycle** — right hand + left hand
- [ ] **Broadcast both damages** — extend `combat:damage` event OR add new event
- [ ] **Double Attack interaction** — only proc on right hand, add left hand hit alongside
- [ ] **Apply mastery penalties** — fetch from `getPassiveSkillBonuses()`
- [ ] **Kill processing** — if right hand kills target, skip left hand; if right hand doesn't kill, left hand damage still applies

### 13.5 Skill System Changes (`ro_skill_data.js`)

- [ ] **Add Righthand Mastery skill definition** — passive, 5 levels, Assassin class
- [ ] **Add Lefthand Mastery skill definition** — passive, 5 levels, prereq: RHM Lv2
- [ ] **Update `getPassiveSkillBonuses()`** — return `rightHandMasteryLv` and `leftHandMasteryLv`
- [ ] **Skill damage with dual wield** — ensure skills use right-hand weapon only for ATK/element

### 13.6 Equipment Restriction Helpers

- [ ] **Update `isTwoHandedWeapon()` or add `isKatar()`** — treat Katar as incompatible with dual wield
- [ ] **Update shield/2H weapon interaction** — Assassin left-hand weapon treated similarly to 2H weapon (blocks shield)

---

## 14. Client Implementation Tasks

### 14.1 Equipment Subsystem (`EquipmentSubsystem.h/.cpp`)

- [ ] **Add `EquipSlots::WeaponLeft` constant** — `TEXT("weapon_left")`
- [ ] **Update `GetValidPositions()`** — for Assassin class, accept weapons in the shield/left-hand position
- [ ] **Update `CanEquipToSlot()`** — allow daggers/swords/axes in shield slot for Assassin
- [ ] **Update `RefreshEquippedSlots()`** — handle `weapon_left` equipped_position from server
- [ ] **Update `UnequipSlot()`** — support unequipping `weapon_left`

### 14.2 Equipment Widget (`SEquipmentWidget.cpp`)

- [ ] **Conditionally label shield slot** — show "Left Hand" for Assassin, "Shield" for others
- [ ] **Accept weapon drag to shield slot** — for Assassin, allow weapon types in shield slot
- [ ] **Display weapon icon in shield slot** — when left-hand weapon is equipped
- [ ] **Handle Katar display** — show Katar in weapon slot, grey out/block shield slot

### 14.3 CharacterData Changes (`CharacterData.h`)

- [ ] **Add `EquippedPosition` to `FInventoryItem`** — to distinguish `weapon` vs `weapon_left`
- [ ] **Parse `equippedPosition` from server** — `inventory:data` already sends this field

### 14.4 Inventory Subsystem (`InventorySubsystem.cpp`)

- [ ] **Parse `weapon_left` equipped position** — add to `ParseItemFromJson()`
- [ ] **Drag state handling** — allow dragging weapons to equipment shield slot for Assassin

### 14.5 Combat Display

- [ ] **DamageNumberSubsystem** — handle dual damage events (two numbers per attack cycle)
- [ ] **WorldHealthBarSubsystem** — apply combined damage for HP bar updates
- [ ] **BasicInfoSubsystem** — no change needed (HP updates come from server)
- [ ] **Combat Stats display** — show dual wield status or combined ATK

### 14.6 Item Tooltip

- [ ] **ItemTooltipBuilder** — no changes needed (weapons show their stats normally)
- [ ] **Equipment panel tooltips** — show "Left Hand" label for off-hand slot

---

## 15. Socket.io Event Changes

### 15.1 Modified Events

**`combat:damage` — Extended Payload:**
```javascript
{
    attackerId, targetId,
    damage,          // Right hand damage
    damage2,         // Left hand damage (0 if not dual wielding)
    isDualWield,     // true if dual wielding
    isCritical,      // Right hand crit
    isCritical2,     // Left hand "crit" (cosmetic mirror of right hand)
    isMiss,          // Right hand miss
    // isMiss2 is NEVER sent (left hand never misses)
    hitType,         // 'normal', 'critical', 'miss'
    hitType2,        // 'normal' (always, for left hand) or absent
    element,         // Right hand element
    element2,        // Left hand element
    isEnemy, targetHealth, targetMaxHealth,
    attackerName, targetName, targetX, targetY, targetZ
}
```

**`inventory:equipped` — Extended Payload:**
```javascript
{
    inventoryId, equipped: true/false,
    slot: 'weapon' | 'weapon_left' | 'shield' | ...,
    equippedPosition: 'weapon' | 'weapon_left' | 'shield' | ...
}
```

**`player:equipment_changed` — Extended:**
```javascript
{
    characterId, slot,
    itemId, weaponType, refineLevel,
    // NEW: left hand info
    leftItemId, leftWeaponType, leftRefineLevel
}
```

### 15.2 No New Events Needed

All dual wield information can be carried in existing events with additional fields. The `damage2` field is the key addition to `combat:damage`.

---

## 16. Database Changes

### 16.1 `character_inventory` Table

No schema changes needed. The `equipped_position` column (VARCHAR(20)) already supports arbitrary position strings. We simply use `'weapon_left'` as a new position value.

### 16.2 Queries to Update

- [ ] **Player join query** — load left-hand weapon info alongside right-hand weapon
- [ ] **Equipment bonus aggregation** — include `equipped_position = 'weapon_left'` items
- [ ] **Weapon element/ATK loading** — load both right and left weapon data
- [ ] **Card bonus rebuild** — rebuild per-hand card mods for auto-attacks

### 16.3 New SQL Patterns

```sql
-- Load both equipped weapons
SELECT ci.*, i.atk, i.weapon_type, i.element, i.weapon_level, i.weapon_range,
       i.aspd_modifier, ci.equipped_position, ci.refine_level
FROM character_inventory ci
JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1
  AND ci.is_equipped = true
  AND ci.equipped_position IN ('weapon', 'weapon_left')
```

---

## 17. Implementation Phases & Checklist

### Phase A: Foundation — Equipment Slot Support
**Goal:** Assassin can equip two weapons, server tracks both, client displays both.

- [ ] A1. Add `weapon_left` to `EquipSlots` namespace in `EquipmentSubsystem.h`
- [ ] A2. Add `canDualWield(jobClass)` helper in `index.js`
- [ ] A3. Add `isValidLeftHandWeapon(weaponType)` helper in `index.js`
- [ ] A4. Modify `inventory:equip` handler for dual-wield equip logic
- [ ] A5. Track `player.weaponATK_L`, `player.weaponType_L`, `player.weaponElement_L`, `player.weaponLevel_L`
- [ ] A6. Load left-hand weapon data on `player:join`
- [ ] A7. Update `inventory:data` to include `equippedPosition: 'weapon_left'` items
- [ ] A8. Client: parse `weapon_left` position in `InventorySubsystem.cpp`
- [ ] A9. Client: update `EquipmentSubsystem` to handle `weapon_left` slot
- [ ] A10. Client: update `SEquipmentWidget` to display left-hand weapon in shield slot
- [ ] A11. Client: label shield slot as "Left Hand" for Assassin class
- [ ] A12. Handle Katar/shield conflicts with left-hand weapon

**Test:** Assassin can equip dagger in right hand, another dagger in left hand. Both show in equipment window. Unequipping either works correctly.

### Phase B: ASPD Calculation
**Goal:** Dual wield ASPD uses the combined formula, attack speed changes correctly.

- [ ] B1. Add `axe` delay to Assassin in `ASPD_BASE_DELAYS`
- [ ] B2. Modify `calculateDerivedStats()` to detect dual wield and use combined WD formula
- [ ] B3. Pass dual wield ASPD to `getAttackIntervalMs()`
- [ ] B4. Update `combat:auto_attack_started` to send correct `attackIntervalMs`
- [ ] B5. Verify ASPD values match expected (compare with examples in Section 5.3)

**Test:** Equipping two daggers shows lower ASPD than one dagger. Attack speed in-game matches the calculated interval.

### Phase C: Dual Hit Auto-Attack
**Goal:** Both weapons deal damage each attack cycle with correct formulas.

- [ ] C1. Add `isDualWielding(player)` detection in auto-attack tick
- [ ] C2. Run physical damage pipeline twice (right + left) per attack cycle
- [ ] C3. Apply mastery penalties (50%/30% base, scaling with skill levels)
- [ ] C4. Left hand: `forceHit: true` (always hits, no FLEE check)
- [ ] C5. Left hand: `noCrit: true` (never crits independently)
- [ ] C6. Extend `combat:damage` event with `damage2`, `isDualWield`, `element2` fields
- [ ] C7. Client: `DamageNumberSubsystem` displays two damage numbers per cycle
- [ ] C8. Client: `WorldHealthBarSubsystem` applies total damage (damage + damage2)
- [ ] C9. Kill processing: apply right hand damage first, then left hand if target still alive
- [ ] C10. If right hand crit: send `isCritical2: true` for left hand (cosmetic)

**Test:** Attacking a monster shows two damage numbers. Right hand can miss/crit. Left hand always hits, never crits. Total damage reduces monster HP correctly.

### Phase D: Mastery Passive Skills
**Goal:** Righthand and Lefthand Mastery skills are defined and affect dual wield damage.

- [ ] D1. Add Righthand Mastery to `ro_skill_data.js` (passive, 5 levels, Assassin)
- [ ] D2. Add Lefthand Mastery to `ro_skill_data.js` (passive, 5 levels, prereq: RHM Lv2)
- [ ] D3. Update `getPassiveSkillBonuses()` to return mastery levels
- [ ] D4. Apply mastery percentages in dual wield damage calculation
- [ ] D5. Verify: 0 mastery = 50%/30%, max mastery = 100%/80%
- [ ] D6. Verify: masteries only affect auto-attacks, not skills
- [ ] D7. Verify: masteries have no effect on Katars
- [ ] D8. Add skill icons for both masteries
- [ ] D9. Add to Assassin skill tree layout

**Test:** Without mastery, dual wield damage is heavily penalized. With max mastery, right hand deals full damage, left hand deals 80%.

### Phase E: Double Attack & Crit Integration
**Goal:** Double Attack and critical hits work correctly with dual wield.

- [ ] E1. Double Attack only procs on right-hand weapon during dual wield
- [ ] E2. When DA procs: 2 right-hand hits + 1 left-hand hit = 3 hits total
- [ ] E3. DA only triggers when right hand is a Dagger
- [ ] E4. Critical: only right hand can crit, left hand damage is always non-crit
- [ ] E5. Visual: if right crits, BOTH hits show crit bubble on client
- [ ] E6. Client: handle 3-hit display for DA + dual wield combo
- [ ] E7. Verify: DA does not proc on left hand even if it's a dagger

**Test:** With DA skilled and dual daggers, some attacks show 3 damage numbers. Crit on right hand shows crit visual on both. Left hand damage never shows crit values.

### Phase F: Card & Element Per-Hand
**Goal:** Each hand uses its own weapon's cards and element for auto-attacks.

- [ ] F1. Track per-hand card bonuses: `player.cardModsRight`, `player.cardModsLeft`
- [ ] F2. Rebuild per-hand card mods on equip/unequip (extend `rebuildCardBonuses()`)
- [ ] F3. Right hand damage pipeline uses `cardModsRight`
- [ ] F4. Left hand damage pipeline uses `cardModsLeft`
- [ ] F5. Each hand uses its own weapon's element for auto-attacks
- [ ] F6. Skills: use merged `cardMods` (both weapons) but only right-hand element/ATK
- [ ] F7. Drake Card in either weapon benefits both hands (nullify size penalty)
- [ ] F8. Endow buffs apply to both weapons' elements

**Test:** Right hand with fire weapon + Hydra card, left hand with holy weapon + Minorous card — each hand's damage reflects its own element and card bonuses correctly.

### Phase G: Skill Interaction
**Goal:** Skills work correctly with dual wield (right hand only, no mastery penalty).

- [ ] G1. Skills use only right-hand weapon ATK, element, and refine
- [ ] G2. Skills do NOT trigger left-hand hit
- [ ] G3. Mastery penalties do NOT apply to skills
- [ ] G4. Katar-only skills (Sonic Blow, Grimtooth) are blocked when dual wielding
- [ ] G5. Card bonuses from both weapons apply to skill damage
- [ ] G6. Verify: Meteor Assault, Venom Splasher work with dual wield

**Test:** Using Meteor Assault with dual wield deals single hit using right-hand ATK. Sonic Blow is blocked (requires Katar).

### Phase H: Polish & Edge Cases
**Goal:** Handle all edge cases and polish the experience.

- [ ] H1. Unequip right hand while dual wielding → left hand becomes "normal" single weapon? Or stays as left-hand-only (unarmed main hand)?
  - **RO behavior:** Left-hand-only = unarmed main hand damage. Implement accordingly.
- [ ] H2. Weight update when equipping/unequipping left-hand weapon
- [ ] H3. `player:stats` broadcast after dual wield equip changes
- [ ] H4. Zone transition: preserve both equipped weapons
- [ ] H5. Death/respawn: both weapons remain equipped
- [ ] H6. Other players see correct weapon visuals (future: animation)
- [ ] H7. Tooltip: combat stats panel shows dual wield info
- [ ] H8. Hotbar: cannot assign left-hand weapon to hotbar (equipment, not consumable)
- [ ] H9. Refine: can refine either weapon independently
- [ ] H10. Card compound: can compound cards into either weapon independently

---

## Appendix A: Source References

| Source | URL | Credibility | Used For |
|--------|-----|-------------|----------|
| iRO Wiki — Dual Wielding | https://irowiki.org/wiki/Dual_Wielding | 5/5 Official wiki | Core mechanics, card/element rules |
| iRO Wiki Classic — Attacks | https://irowiki.org/classic/Attacks | 5/5 Classic archive | Attack pipeline, left hand as skill |
| iRO Wiki Classic — ASPD | https://irowiki.org/classic/ASPD | 5/5 Classic archive | ASPD formula, dual wield BTBA |
| iRO Wiki — Righthand Mastery | https://irowiki.org/wiki/Righthand_Mastery | 5/5 Official wiki | Mastery levels, damage % |
| iRO Wiki — Lefthand Mastery | https://irowiki.org/wiki/Lefthand_Mastery | 5/5 Official wiki | Mastery levels, damage % |
| iRO Wiki Classic — Righthand Mastery | https://irowiki.org/classic/Righthand_Mastery | 5/5 Classic archive | Pre-renewal base penalty |
| iRO Wiki — Double Attack | https://irowiki.org/wiki/Double_Attack | 5/5 Official wiki | DA + dual wield interaction |
| iRO Wiki — Sonic Blow | https://irowiki.org/wiki/Sonic_Blow | 5/5 Official wiki | Katar-only skill confirmation |
| iRO Wiki Classic — Assassin | https://irowiki.org/classic/Assassin | 5/5 Classic archive | Class weapon types, slot layout |
| RagnaPlace — Dual Wielding | https://ragnaplace.com/en/wiki/irowiki/Dual_Wielding | 4/5 Mirror | Card/element/skill rules |
| RateMyServer Forum — DD Details | https://forum.ratemyserver.net/general-discussion/double-dagger-details/ | 3/5 Community | Damage formulas, card stacking |
| rAthena Forum — LH Damage | https://rathena.org/board/topic/113069-left-hand-on-dual-wield-reduces-damage/ | 4/5 Emulator source | ATK_RATER/ATK_RATEL formulas |
| rAthena GitHub — Issue #8284 | https://github.com/rathena/rathena/issues/8284 | 4/5 Emulator source | Offhand statusATK behavior |
| rAthena Forum — DW for All Jobs | https://rathena.org/board/topic/104092-dual-wielding-for-all-jobs/ | 4/5 Emulator source | ASPD combined formula |
| MyRoLife — Katar vs DD | https://myrolife.blogspot.com/2016/02/assassin-katar-vs-dual-dagger.html | 3/5 Community guide | Build comparison |

## Appendix B: Known Ambiguities

1. **Left hand Perfect Dodge:** Whether left-hand "skill attacks" are affected by Perfect Dodge is unclear. rAthena treats skill attacks as bypassing PD. **Decision:** Left hand bypasses PD (consistent with skill attack behavior).

2. **Left hand damage display when "miss":** Some sources say left hand NEVER shows "Miss!", while the damage is still calculated. Since left hand always hits (forceHit), this is moot — it always deals damage. **Decision:** Never send `isMiss` for left hand.

3. **Card application scope:** Some sources say "all cards apply to the right-hand weapon only" while others say cards from both weapons apply to both hands for auto-attacks. The rAthena code runs the full pipeline per-weapon, meaning each hand uses its own weapon's cards. **Decision:** Per-hand card application for auto-attacks. Merged card application for skills.

4. **Endow buff on left hand:** Whether Sage endow skills change both weapons' elements is not fully confirmed. Most sources suggest endow applies to BOTH. **Decision:** Endow applies to both weapons.

5. **Assassin Cross of Sunset / Enchant Deadly Poison:** These Assassin Cross skills' interaction with dual wield is not fully documented for pre-renewal. **Decision:** Defer to when Assassin Cross class is implemented.

## Appendix C: Files To Modify

| File | Changes |
|------|---------|
| `server/src/index.js` | Equip handler, auto-attack tick, player join, stat recalc, equipment broadcast |
| `server/src/ro_damage_formulas.js` | Dual wield damage pipeline, mastery penalty application |
| `server/src/ro_exp_tables.js` | Add axe delay to Assassin, dual wield ASPD formula |
| `server/src/ro_skill_data.js` | Righthand/Lefthand Mastery definitions |
| `server/src/ro_card_effects.js` | Per-hand card bonus tracking (if needed) |
| `client/.../CharacterData.h` | `EquippedPosition` field, dual wield data |
| `client/.../UI/EquipmentSubsystem.h/.cpp` | `WeaponLeft` slot, `CanEquipToSlot()` |
| `client/.../UI/SEquipmentWidget.cpp` | Shield→Left Hand label, weapon drag to shield slot |
| `client/.../UI/InventorySubsystem.cpp` | Parse `weapon_left` position |
| `client/.../UI/DamageNumberSubsystem.cpp` | Handle `damage2` field, dual damage display |
| `client/.../UI/WorldHealthBarSubsystem.cpp` | Apply combined damage |
| `client/.../UI/CombatStatsSubsystem.cpp` | Show dual wield stats |
| `database/migrations/` | No schema changes needed (VARCHAR position supports `weapon_left`) |
