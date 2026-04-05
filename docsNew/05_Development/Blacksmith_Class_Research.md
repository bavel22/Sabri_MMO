# Blacksmith Class Research -- Complete Pre-Renewal Implementation Guide

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Blacksmith_Skills_Audit](Blacksmith_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Scope:** Blacksmith (2nd class from Merchant) + Whitesmith/Mastersmith (Transcendent)
**Sources:** iRO Wiki, iRO Wiki Classic, rAthena pre-renewal DB, RateMyServer, rAthena source code

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Blacksmith Combat Skills (12)](#blacksmith-combat-skills)
3. [Blacksmith Forging Skills (12)](#blacksmith-forging-skills)
4. [Blacksmith Quest Skills (2)](#blacksmith-quest-skills)
5. [Skill Tree Prerequisites](#skill-tree-prerequisites)
6. [Existing Implementation Audit](#existing-implementation-audit)
7. [Forging System Architecture](#forging-system-architecture)
8. [Refining System Architecture](#refining-system-architecture)
9. [Cart System Architecture](#cart-system-architecture)
10. [New Systems Required](#new-systems-required)
11. [Whitesmith/Mastersmith Skills (Trans)](#whitesmithmastersmith-skills)
12. [Implementation Priority](#implementation-priority)
13. [Integration Points](#integration-points)
14. [Constants and Data Tables](#constants-and-data-tables)

---

## Class Overview

**Blacksmith** is the 2nd class advancement from Merchant. It is a melee physical class that specializes in:

- **Self-buffing combat** -- Adrenaline Rush (ASPD), Power Thrust (ATK%), Weapon Perfection (size penalty removal), Maximize Power (max variance)
- **AoE crowd control** -- Hammer Fall (5x5 stun)
- **Weapon forging** -- Craft elemental weapons, star crumb weapons, material refinement
- **Equipment repair** -- Fix broken weapons/armor for party members
- **Passive enhancements** -- Weaponry Research (+ATK/+HIT), Skin Tempering (fire/neutral resist), Hilt Binding (+STR/+ATK/duration bonus)

**Class progression:**
```
Novice -> Merchant -> Blacksmith -> Whitesmith (Mastersmith)
```

**Primary stats:** STR (damage), DEX (hit, forge rate), AGI (ASPD), VIT (survival), LUK (forge rate, crit)

**Primary weapons:** Axes, Maces (required for Adrenaline Rush)

**Build types:**
- **Battlesmith (Combat):** STR/AGI/DEX focus, Adrenaline Rush + Power Thrust + Weapon Perfection + Maximize Power
- **Pure Forger:** DEX/LUK focus, all forging skills, minimal combat
- **Battle Forger (Hybrid):** Moderate STR/DEX/LUK, combat + forging

---

## Blacksmith Combat Skills

### Skill ID Range: 1200-1211 (Sabri_MMO internal)
### rAthena Skill ID Range: 105-114 + quest skills 1012-1013

---

### 1. Adrenaline Rush (ID 1200, rA: 111)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (affects party) |
| Element | Neutral |
| Range | 0 (self) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Weapon Requirement | Axe or Mace ONLY |
| Prerequisites | Hammer Fall Lv2 |
| Unlocks | Weapon Perfection (Lv2), Power Thrust (Lv3) |

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 20 | 23 | 26 | 29 | 32 |
| Duration (s) | 30 | 60 | 90 | 120 | 150 |

**SP formula:** `17 + 3 * SkillLevel`
**Duration formula:** `30 * SkillLevel` seconds

**ASPD Bonus Mechanics (Pre-Renewal):**
- **Caster (Blacksmith/Whitesmith):** ASPD delay reduced by 30% (i.e., attacks 30% faster)
- **Party members (any class with Axe/Mace equipped):** ASPD delay reduced by 20%
- Party members without Axe/Mace get NO benefit
- The buff is applied as a multiplicative ASPD modifier: `aspdMultiplier = 1.3` for caster, `1.2` for party

**Implementation as buff:**
```js
buffName: 'adrenaline_rush'
effects: {
    aspdMultiplier: 1.3,   // for caster (Blacksmith)
    // OR aspdMultiplier: 1.2  // for party members
    weaponRestriction: ['axe', 'one_hand_axe', 'two_hand_axe', 'mace']
}
```

**Hilt Binding interaction:** If caster has Hilt Binding Lv1, duration is extended by 10%:
```
finalDuration = baseDuration * (hasHiltBinding ? 1.1 : 1.0)
```

**Does NOT stack with:** Two-Hand Quicken, Spear Quicken, One-Hand Quicken (mutually exclusive ASPD buffs)

---

### 2. Weapon Perfection (ID 1201, rA: 112)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (affects party) |
| Element | Neutral |
| Range | 0 (self) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Adrenaline Rush Lv2 AND Weaponry Research Lv2 |
| Unlocks | Maximize Power (Lv3) |

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 18 | 16 | 14 | 12 | 10 |
| Duration (s) | 10 | 20 | 30 | 40 | 50 |

**SP formula:** `20 - 2 * SkillLevel`
**Duration formula:** `10 * SkillLevel` seconds

**Effect:** Removes ALL size penalties from weapon damage for the caster AND party members on screen. All attacks deal 100% damage regardless of target size (Small/Medium/Large).

**Implementation as buff:**
```js
buffName: 'weapon_perfection'
effects: {
    noSizePenalty: true   // SIZE_PENALTY always returns 100
}
```

**Damage pipeline integration:** In `calculatePhysicalDamage()`, check for `attacker.noSizePenalty` or `attacker.cardNoSizeFix` (Drake Card does the same thing). Already partially implemented via `cardNoSizeFix`.

**Hilt Binding interaction:** Duration extended by 10% if caster has Hilt Binding.

**Comparison with Drake Card:**
- Drake Card: Permanent, weapon-slot only, single person
- Weapon Perfection: Temporary, party-wide, no equipment slot cost

---

### 3. Power Thrust / Over Thrust (ID 1202, rA: 113)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (affects party) |
| Element | Neutral |
| Range | 0 (self) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Adrenaline Rush Lv3 |
| Unlocks | Maximize Power (Lv2) |

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 18 | 16 | 14 | 12 | 10 |
| Duration (s) | 20 | 40 | 60 | 80 | 100 |
| Self ATK% | +5% | +10% | +15% | +20% | +25% |
| Party ATK% | +5% | +5% | +10% | +10% | +15% |

**SP formula:** `20 - 2 * SkillLevel`
**Duration formula:** `20 * SkillLevel` seconds
**Self ATK formula:** `5 * SkillLevel` %
**Party ATK formula (iRO Renewal):** Lv1-2: +5%, Lv3-4: +10%, Lv5: +15%

**Pre-Renewal note:** In pre-renewal, the self ATK boost was `5 * SkillLevel` %. The party values may differ between servers. The rAthena pre-renewal source uses `5 * SkillLevel` % for both self and party.

**Weapon Break Chance:** 0.1% chance per physical attack while active to break the caster's own weapon. This risk applies to ALL attacks (auto-attack and skills).
```js
if (player.activeBuffs.find(b => b.name === 'power_thrust')) {
    if (Math.random() < 0.001) {
        // Weapon breaks -- set player.weaponBroken = true
        // Remove weapon ATK, element, etc. until repaired
    }
}
```

**Implementation as buff:**
```js
buffName: 'power_thrust'
effects: {
    atkPercent: 5 * skillLevel,     // +5% to +25% ATK multiplier
    weaponBreakChance: 0.001        // 0.1% per attack
}
```

**Hilt Binding interaction:** Duration extended by 10% if caster has Hilt Binding.

**Stacking with Maximum Over Thrust:** The two are separate buffs that stack multiplicatively. A Whitesmith with Power Thrust Lv5 (+25%) and Maximum Over Thrust Lv5 (+100%) would deal: `ATK * 1.25 * 2.0 = ATK * 2.50` (250% total).

---

### 4. Maximize Power (ID 1203, rA: 114)

| Property | Value |
|----------|-------|
| Type | Toggle (Active) |
| Max Level | 5 |
| Target | Self |
| Element | Neutral |
| Range | 0 (self) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Weapon Perfection Lv3 AND Power Thrust Lv2 |
| Initial SP Cost | 10 SP (on activation) |

**SP Drain per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Drain | 1 SP per 1s | 1 SP per 2s | 1 SP per 3s | 1 SP per 4s | 1 SP per 5s |

**Effect:** While active, ALL physical attacks (auto-attacks and skills) deal maximum weapon damage -- no weapon variance roll. This means the weapon always deals its maximum ATK value instead of rolling between min and max.

**Duration:** Until SP reaches 0, player recasts to toggle off, or player dies.

**Implementation:**
```js
// Toggle on: deduct 10 SP, set flag
player.maximizePower = true;
player.maximizePowerLevel = learnedLevel;
player.maximizePowerDrainInterval = learnedLevel * 1000; // 1000-5000ms
player.maximizePowerLastDrain = Date.now();

// Toggle off: clear flag
player.maximizePower = false;

// In combat tick (SP drain check):
if (player.maximizePower) {
    const now = Date.now();
    if (now - player.maximizePowerLastDrain >= player.maximizePowerDrainInterval) {
        player.mana = Math.max(0, player.mana - 1);
        player.maximizePowerLastDrain = now;
        if (player.mana <= 0) {
            player.maximizePower = false;
            // Remove buff, broadcast buff_removed
        }
    }
}

// In damage calculation:
if (attacker.maximizePower) {
    variancedWeaponATK = sizedWeaponATK; // Always max, no variance roll
}
```

**Important limitation:** This skill only affects base weapon variance. It does NOT affect over-upgrade variance. A +10 weapon will still have fluctuating over-upgrade bonus damage.

**Does NOT affect:** Magical damage (MATK). The magic equivalent is Recognized Spell (Sage/Scholar skill).

---

### 5. Weaponry Research (ID 1204, rA: 107)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Target | None |
| Prerequisites | Hilt Binding Lv1 |
| Unlocks | Weapon Repair (Lv1), Weapon Perfection (Lv2 with Adrenaline Rush Lv2) |

**Bonus per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | +2 | +4 | +6 | +8 | +10 | +12 | +14 | +16 | +18 | +20 |
| +HIT | +2 | +4 | +6 | +8 | +10 | +12 | +14 | +16 | +18 | +20 |
| Forge Rate | +1% | +2% | +3% | +4% | +5% | +6% | +7% | +8% | +9% | +10% |

**ATK formula:** `+2 * SkillLevel` flat ATK (mastery ATK, bypasses element/size/card)
**HIT formula:** `+2 * SkillLevel` flat HIT bonus
**Forge Rate formula:** `+1% * SkillLevel` added to forging success rate

**Implementation in `getPassiveSkillBonuses()`:**
```js
const weapResLv = learned[1204] || 0;
if (weapResLv > 0) {
    bonuses.bonusATK += weapResLv * 2;   // Mastery ATK (flat, all weapons)
    bonuses.bonusHIT += weapResLv * 2;   // Flat HIT bonus
}
```

**Note:** Unlike Sword Mastery which only works with swords, Weaponry Research applies to ALL weapon types.

---

### 6. Skin Tempering (ID 1205, rA: 109)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Target | None |
| Prerequisites | None |
| Unlocks | Melt Down (via Whitesmith) |

**Resistance per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Fire Resist | +4% | +8% | +12% | +16% | +20% |
| Neutral Resist | +1% | +2% | +3% | +4% | +5% |

**Fire Resist formula:** `4 * SkillLevel` % reduction to Fire element damage
**Neutral Resist formula:** `1 * SkillLevel` % reduction to Neutral element damage

**Note:** The in-game description incorrectly states 5% Fire per level. The actual value is 4% per level (confirmed by iRO Wiki, rAthena source).

**Implementation in `getPassiveSkillBonuses()`:**
```js
const skinTempLv = learned[1205] || 0;
if (skinTempLv > 0) {
    bonuses.elementResist = bonuses.elementResist || {};
    bonuses.elementResist.fire = (bonuses.elementResist.fire || 0) + skinTempLv * 4;
    bonuses.elementResist.neutral = (bonuses.elementResist.neutral || 0) + skinTempLv * 1;
}
```

**Damage pipeline integration:** After element modifier lookup, subtract resistance:
```js
// After ELEMENT_TABLE lookup:
const eleResist = (target.elementResist && target.elementResist[atkElement]) || 0;
eleModifier = Math.max(0, eleModifier - eleResist);
```

---

### 7. Hammer Fall (ID 1206, rA: 110)

| Property | Value |
|----------|-------|
| Type | Active, Offensive (status only, NO damage) |
| Max Level | 5 |
| Target | Ground (5x5 AoE) |
| Element | Neutral |
| Range | 1 cell (melee, ~150 UE units) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 (rAthena pre-re: 0; our code has 1000 -- BUG) |
| SP Cost | 10 (all levels) |
| Weapon Requirement | Axe or Mace ONLY |
| Prerequisites | None |
| Unlocks | Adrenaline Rush (Lv2) |

**Stun Chance per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Stun% | 30% | 40% | 50% | 60% | 70% |

**Stun formula:** `20 + 10 * SkillLevel` %
**Stun duration:** 5 seconds (reduced by target VIT, standard stun resistance formula)
**AoE radius:** 5x5 cells = ~250 UE units from center

**IMPORTANT: Hammer Fall deals NO damage.** It ONLY inflicts Stun status on enemies in the AoE. This is different from most AoE skills that deal damage + status.

**Stun resistance (standard formula):**
```
FinalChance = BaseChance - (BaseChance * TargetVIT / 100) + srcBaseLv - tarBaseLv - tarLUK
```

**Implementation:**
```js
if (skill.name === 'hammer_fall') {
    if (!hasGroundPos) { socket.emit('skill:error', { message: 'No ground position' }); return; }
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const stunChance = effectVal; // 30-70%
    const AOE_RADIUS = 250; // 5x5 cells
    const zone = player.zone;

    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== zone) continue;
        const dist = calculateDistance({ x: groundX, y: groundY }, { x: enemy.x, y: enemy.y });
        if (dist <= AOE_RADIUS) {
            // Apply stun via ro_status_effects.js
            const result = applyStatusEffect(
                { level: player.level, luk: player.luk },
                enemy, 'stun', stunChance, { duration: 5000 }
            );
            if (result.applied) {
                broadcastToZone(zone, 'status:applied', {
                    targetId: eid, isEnemy: true, statusType: 'stun', duration: result.duration
                });
            }
        }
    }
    // NO skill:effect_damage broadcast -- this skill does no damage
    socket.emit('skill:used', { ... });
}
```

---

### 8. Hilt Binding (ID 1207, rA: 105)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 1 |
| Target | None |
| Prerequisites | None |
| Unlocks | Weaponry Research (Lv1), Ore Discovery (Lv1 with Steel Tempering Lv1) |

**Effects:**
- **+1 STR** (permanent stat bonus)
- **+4 ATK** (flat mastery ATK, bypasses multipliers)
- **+10% duration** to Adrenaline Rush, Power Thrust, and Weapon Perfection

**Implementation in `getPassiveSkillBonuses()`:**
```js
const hiltLv = learned[1207] || 0;
if (hiltLv > 0) {
    bonuses.bonusSTR = (bonuses.bonusSTR || 0) + 1;
    bonuses.bonusATK += 4;
    bonuses.hiltBindingDurationBonus = true; // +10% to specific buff durations
}
```

**Duration extension logic:** When applying Adrenaline Rush, Power Thrust, or Weapon Perfection buffs, check if caster has Hilt Binding:
```js
const hiltBonus = getPassiveSkillBonuses(player).hiltBindingDurationBonus ? 1.1 : 1.0;
const finalDuration = Math.floor(baseDuration * hiltBonus);
```

---

### 9. Ore Discovery (ID 1208, rA: 106)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 1 |
| Target | None |
| Prerequisites | Steel Tempering Lv1 AND Hilt Binding Lv1 |

**Effect:** Gives a chance to find additional ore items when killing monsters. The exact chance is not precisely documented in rAthena, but it triggers on monster kill and drops an ore item from the ore group.

**Implementation (deferred -- requires loot drop enhancement):**
```js
// In processEnemyDeathFromSkill():
const oreDiscLv = learned[1208] || 0;
if (oreDiscLv > 0 && Math.random() < 0.05) { // ~5% chance (estimated)
    // Drop random ore: Iron Ore, Coal, etc.
    // Add to loot pool alongside normal drops
}
```

**Priority:** LOW -- cosmetic/economic skill, not combat-critical.

---

### 10. Weapon Repair (ID 1209, rA: 108)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 1 |
| Target | Single ally (friendly target) |
| Element | Neutral |
| Range | 2 cells (~150 UE units) |
| Cast Time | 5000ms (pre-renewal: 7500ms on some sources, rAthena uses 5000ms) |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| SP Cost | 30 |
| Prerequisites | Weaponry Research Lv1 |

**Materials Required (consumed on use):**

| Equipment Type | Material Required |
|---------------|-------------------|
| Weapon Lv1 | 1 Iron Ore |
| Weapon Lv2 | 1 Iron |
| Weapon Lv3 | 1 Steel |
| Weapon Lv4 | 1 Rough Oridecon |
| Armor (any) | 1 Steel |

**Effect:** Repairs a broken weapon or armor on the target player, restoring it to usable condition. The material is consumed from the Blacksmith's inventory.

**Implementation (deferred -- requires weapon break system):**
- Weapon break system must exist first (Power Thrust weapon break, Melt Down, etc.)
- `player.weaponBroken = true/false` flag
- Broken weapon: ATK = 0, element = neutral, all bonuses removed
- Repair: restore weapon stats, consume material

**Priority:** LOW until weapon break system is implemented.

---

### 11. Greed (ID 1210, rA: 1013)

| Property | Value |
|----------|-------|
| Type | Active (Quest Skill) |
| Max Level | 1 |
| Target | Self |
| Element | Neutral |
| Range | 0 (self-centered) |
| AoE | 5x5 cells (~250 UE units) |
| Cast Time | 0 |
| After-Cast Delay | 1000ms |
| Cooldown | 0 |
| SP Cost | 10 |
| Prerequisites | Quest completion (Job Level 30+) |

**Effect:** Automatically pick up ALL item drops within 2 cells (5x5 area) of the caster. This is essential for Battlesmiths who fight large groups.

**Restrictions:**
- Cannot be used in towns, PvP maps, or War of Emperium maps
- Quest skill (not from skill tree)

**Implementation (deferred -- requires ground loot system):**
Currently, loot drops go directly to inventory via `inventory:loot_drop`. A full ground-loot system where items appear on the ground first would need to be implemented for Greed to have meaning. Until then, this skill has no functional purpose.

**Priority:** DEFERRED -- depends on ground loot system (Phase 10+).

---

### 12. Dubious Salesmanship / Unfair Trick (ID 1211, rA: 1012)

| Property | Value |
|----------|-------|
| Type | Passive (Quest Skill) |
| Max Level | 1 |
| Target | None |
| Prerequisites | Quest completion (Job Level 25+) |

**Effect (Pre-Renewal):** Reduces Mammonite Zeny cost by 10%.

```js
// In Mammonite handler:
const dubSalesLv = (player.learnedSkills || {})[1211] || 0;
const zenyCostReduction = dubSalesLv > 0 ? 0.9 : 1.0;
const finalZenyCost = Math.floor(learnedLevel * 100 * zenyCostReduction);
```

**Priority:** LOW -- trivial to implement, minor economic benefit.

---

## Blacksmith Forging Skills

### Skill ID Range: 1220-1230 (Sabri_MMO internal)
### rAthena Skill ID Range: 94-104

All forging skills are **Passive** -- they unlock the ability to forge specific weapon types or refine materials. The actual forging action is performed via a crafting UI (not a combat skill).

---

### 13. Iron Tempering (ID 1220, rA: 94)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | None |
| Unlocks | Steel Tempering (Lv1), Enchanted Stone Craft (Lv1) |

**Recipe:** 1 Iron Ore + Mini Furnace -> 1 Iron

**Success Rate per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Base Rate | 45% | 50% | 55% | 60% | 65% |

**Formula:** `40 + 5 * SkillLevel` % base, plus `JobLevel * 0.2 + DEX * 0.1 + LUK * 0.1`

---

### 14. Steel Tempering (ID 1221, rA: 95)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Iron Tempering Lv1 |
| Unlocks | Ore Discovery (Lv1 with Hilt Binding Lv1) |

**Recipe:** 5 Iron + 1 Coal -> 1 Steel

**Success Rate per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Base Rate | 35% | 40% | 45% | 50% | 55% |

---

### 15. Enchanted Stone Craft (ID 1222, rA: 96)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Iron Tempering Lv1 |

**Recipes (10 elemental ores -> 1 elemental stone):**

| Recipe | Product |
|--------|---------|
| 10 Flame Heart Ore | 1 Flame Heart (Fire Stone) |
| 10 Mystic Frozen Ore | 1 Mystic Frozen (Water Stone) |
| 10 Rough Wind Ore | 1 Rough Wind (Wind Stone) |
| 10 Great Nature Ore | 1 Great Nature (Earth Stone) |

**Success Rate per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Base Rate | 15% | 20% | 25% | 30% | 35% |

---

### 16. Research Oridecon (ID 1223, rA: 97)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Enchanted Stone Craft Lv1 |

**Effect:** Increases the success rate when forging Level 3 weapons (which require Oridecon). Each level adds a flat bonus to the forging success rate for Lv3 weapons only.

**Bonus per Level:** +100 to `make_per` (roughly +1% per level)

---

### 17-23. Smith Skills (IDs 1224-1230)

Seven weapon forging skills, each with 3 levels. Each level adds +5% to the forging success rate for that weapon category (rAthena: +500 to `make_per`).

| ID | Skill | rA ID | Prerequisites | Weapons Forged |
|----|-------|-------|---------------|----------------|
| 1224 | Smith Dagger | 98 | None (or Weaponry Research Lv1*) | Knife, Main Gauche, Dirk, Dagger, Stiletto, Gladius, Damascus |
| 1225 | Smith Sword | 99 | Smith Dagger Lv1 | Sword, Falchion, Blade, Lapier, Scimitar, Ring Pommel Saber, Saber, Haedongeum, Tsurugi, Flamberge |
| 1226 | Smith Two-Handed Sword | 100 | Smith Sword Lv1 | Katana, Slayer, Bastard Sword, Two-Handed Sword, Broad Sword, Claymore |
| 1227 | Smith Axe | 101 | Smith Sword Lv2 | Axe, Battle Axe, Hammer, Buster, Two-Handed Axe |
| 1228 | Smith Mace | 102 | Smith Knucklebrace Lv1 | Club, Mace, Smasher, Flail, Chain, Morning Star |
| 1229 | Smith Knucklebrace | 103 | Smith Dagger Lv1 | Knuckle Duster, Brass Knuckle, Waghnak, Hora, Fist, Claw |
| 1230 | Smith Spear | 104 | Smith Dagger Lv2 | Javelin, Spear, Pike, Guisarme, Glaive, Partizan, Trident |

*Note: Our code has Smith Dagger requiring Weaponry Research Lv1 as a prerequisite, but the RateMyServer skill simulator shows no prerequisite. The correct prerequisite chain according to rAthena is: Smith Dagger has NO prerequisites.

**Success Rate per Level (for each Smith skill):**

| Level | 1 | 2 | 3 |
|-------|---|---|---|
| Forge Rate Bonus | +10% | +20% | +30% |

---

## Blacksmith Quest Skills

### Greed (ID 1210) and Dubious Salesmanship (ID 1211)

See skills #11 and #12 above.

---

## Skill Tree Prerequisites

### Complete Blacksmith Prerequisite Chain

```
NO PREREQS:                HILT BINDING (1207)
    |                          |
    v                          v
HAMMER FALL (1206)      WEAPONRY RESEARCH (1204) --- Lv1 ---> WEAPON REPAIR (1209)
    |                          |
    | Lv2                      | Lv2
    v                          |
ADRENALINE RUSH (1200) <--- + --- (need both AR Lv2 + WR Lv2)
    |           |                          v
    | Lv3       | Lv2           WEAPON PERFECTION (1201) --- Lv3 ---|
    v           |                                                    |
POWER THRUST (1202)                                                  |
    |                                                                |
    | Lv2                                                            |
    v                                                                v
    +--- (need both PT Lv2 + WP Lv3) ---> MAXIMIZE POWER (1203)


NO PREREQS:                   HILT BINDING (1207) + STEEL TEMPERING (1221)
    |                                  |
    v                                  v
SKIN TEMPERING (1205)         ORE DISCOVERY (1208)

IRON TEMPERING (1220) --- Lv1 ---> STEEL TEMPERING (1221)
    |                               ENCHANTED STONE CRAFT (1222) --- Lv1 ---> RESEARCH ORIDECON (1223)

SMITH DAGGER (1224) --- Lv1 ---> SMITH SWORD (1225) --- Lv1 ---> SMITH TWO-HANDED SWORD (1226)
    |                               |
    | Lv1                           | Lv2
    v                               v
SMITH KNUCKLEBRACE (1229)       SMITH AXE (1227)
    |
    | Lv1
    v
SMITH MACE (1228)

SMITH DAGGER (1224) --- Lv2 ---> SMITH SPEAR (1230)
```

### Prerequisites Table (Corrected vs Current Code)

| Skill | rAthena Prerequisites | Current Code Prerequisites | Status |
|-------|----------------------|---------------------------|--------|
| Adrenaline Rush (1200) | Hammer Fall Lv2 | None | **BUG -- missing prereq** |
| Weapon Perfection (1201) | Adrenaline Rush Lv2 + Weaponry Research Lv2 | Adrenaline Rush Lv2 only | **BUG -- missing WR Lv2** |
| Power Thrust (1202) | Adrenaline Rush Lv3 | Adrenaline Rush Lv5 | **BUG -- wrong level (5 should be 3)** |
| Maximize Power (1203) | Weapon Perfection Lv3 + Power Thrust Lv2 | Power Thrust Lv5 + Weapon Perfection Lv3 | **BUG -- PT should be Lv2 not Lv5** |
| Weaponry Research (1204) | Hilt Binding Lv1 | Hilt Binding Lv1 | CORRECT |
| Skin Tempering (1205) | None | None | CORRECT |
| Hammer Fall (1206) | None | None | CORRECT |
| Hilt Binding (1207) | None | None | CORRECT |
| Ore Discovery (1208) | Steel Tempering Lv1 + Hilt Binding Lv1 | Steel Tempering Lv1 + Hilt Binding Lv1 | CORRECT |
| Weapon Repair (1209) | Weaponry Research Lv1 | Weaponry Research Lv1 | CORRECT |
| Smith Dagger (1224) | None | Weaponry Research Lv1 | **BUG -- should have no prereq** |
| Smith Sword (1225) | Smith Dagger Lv1 | Smith Dagger Lv1 | CORRECT |
| Smith Two-Handed Sword (1226) | Smith Sword Lv1 | Smith Sword Lv1 | CORRECT |
| Smith Axe (1227) | Smith Sword Lv2 | Smith Dagger Lv1 | **BUG -- should be Smith Sword Lv2** |
| Smith Mace (1228) | Smith Knucklebrace Lv1 | Smith Dagger Lv1 | **BUG -- should be Smith Knucklebrace Lv1** |
| Smith Knucklebrace (1229) | Smith Dagger Lv1 | Smith Mace Lv1 | **BUG -- prereq reversed** |
| Smith Spear (1230) | Smith Dagger Lv2 | Smith Dagger Lv1 | **BUG -- should be Lv2 not Lv1** |
| Research Oridecon (1223) | Enchanted Stone Craft Lv1 | Iron Tempering Lv1 | **BUG -- should be ESC Lv1** |

**Total prerequisite bugs found: 9**

---

## Existing Implementation Audit

### Current State in `ro_skill_data_2nd.js`

The Blacksmith class has 24 skill definitions (IDs 1200-1230) already in the codebase. All definitions exist, but many have prerequisite errors and some have incorrect cooldown/effectValue values.

### Audit Results

| Skill | Definition | Prerequisites | Handler | VFX | Status |
|-------|-----------|---------------|---------|-----|--------|
| Adrenaline Rush (1200) | Exists | WRONG (missing HF Lv2) | None | None | Needs fix + handler |
| Weapon Perfection (1201) | Exists | WRONG (missing WR Lv2) | None | None | Needs fix + handler |
| Power Thrust (1202) | Exists | WRONG (AR Lv5 -> Lv3) | None | None | Needs fix + handler |
| Maximize Power (1203) | Exists | WRONG (PT Lv5 -> Lv2) | None | None | Needs fix + handler |
| Weaponry Research (1204) | Exists | CORRECT | None (passive) | N/A | Needs passive handler |
| Skin Tempering (1205) | Exists | CORRECT | None (passive) | N/A | Needs passive handler |
| Hammer Fall (1206) | Exists | CORRECT | None | None | Needs handler |
| Hilt Binding (1207) | Exists | CORRECT | None (passive) | N/A | Needs passive handler |
| Ore Discovery (1208) | Exists | CORRECT | None (passive) | N/A | DEFERRED |
| Weapon Repair (1209) | Exists | CORRECT | None | None | DEFERRED |
| Greed (1210) | Exists | N/A (quest) | None | None | DEFERRED |
| Dubious Sales (1211) | Exists | N/A (quest) | None (passive) | N/A | Needs integration |
| Iron Tempering (1220) | Exists | CORRECT | None (passive) | N/A | DEFERRED (forge system) |
| Steel Tempering (1221) | Exists | CORRECT | None (passive) | N/A | DEFERRED (forge system) |
| Enchanted Stone (1222) | Exists | CORRECT | None (passive) | N/A | DEFERRED (forge system) |
| Research Oridecon (1223) | Exists | WRONG (should be ESC Lv1) | None (passive) | N/A | DEFERRED (forge system) |
| Smith Dagger (1224) | Exists | WRONG (has WR Lv1, should be none) | None (passive) | N/A | DEFERRED (forge system) |
| Smith Sword (1225) | Exists | CORRECT | None (passive) | N/A | DEFERRED (forge system) |
| Smith Two-Handed Sword (1226) | Exists | CORRECT | None (passive) | N/A | DEFERRED (forge system) |
| Smith Axe (1227) | Exists | WRONG (has SD Lv1, should be SS Lv2) | None (passive) | N/A | DEFERRED (forge system) |
| Smith Mace (1228) | Exists | WRONG (has SD Lv1, should be SKB Lv1) | None (passive) | N/A | DEFERRED (forge system) |
| Smith Knucklebrace (1229) | Exists | WRONG (has SM Lv1, should be SD Lv1) | None (passive) | N/A | DEFERRED (forge system) |
| Smith Spear (1230) | Exists | WRONG (has SD Lv1, should be SD Lv2) | None (passive) | N/A | DEFERRED (forge system) |

### Additional Issues in Skill Definitions

| Issue | Skill | Current Value | Correct Value |
|-------|-------|---------------|---------------|
| Hammer Fall has cooldown | 1206 | `cooldown: 1000` | `cooldown: 0` |
| Power Thrust effectValue wrong | 1202 | `effectValue: 10+i*5` (10-30%) | `effectValue: 5+i*5` (5-25%) |
| Power Thrust SP cost pattern | 1202 | `spCost: 18-i*2` | Correct (18/16/14/12/10) |

---

## Forging System Architecture

### Overview

The forging system allows Blacksmiths to create weapons from raw materials. This is a major crafting system that requires:
- Crafting UI (client-side)
- Recipe database (server-side)
- Success rate calculation
- Material consumption
- Forged weapon generation with crafter name, element, star crumbs

### Forging Success Rate Formula (rAthena Source)

```js
// Base rate (out of 10000 = 100.00%)
let make_per = 5000;  // 50% base

// Stat bonuses
make_per += jobLevel * 20;     // +0.2% per job level (max: +10% at JLv50)
make_per += DEX * 10;          // +0.1% per DEX
make_per += LUK * 10;          // +0.1% per LUK

// Smith skill bonus (per weapon type)
make_per += smithSkillLevel * 500;  // +5% per Smith skill level (max +15% at Lv3)

// Weaponry Research bonus
make_per += weaponryResearchLevel * 100;  // +1% per level (max +10% at Lv10)

// Oridecon Research bonus (Lv3 weapons ONLY)
if (weaponLevel === 3) {
    make_per += orideconResearchLevel * 100;  // +1% per level (max +5%)
}

// Anvil bonus
// Standard Anvil:   +0
// Oridecon Anvil:    +300  (+3%)
// Golden Anvil:      +500  (+5%)
// Emperium Anvil:    +1000 (+10%)
make_per += anvilBonus;

// Penalties
if (hasElementStone) make_per -= 2000;  // -20% for elemental weapon
make_per -= numStarCrumbs * 1500;       // -15% per Star Crumb (max 3)
make_per -= (weaponLevel - 1) * 1000;   // -10% per weapon level above 1

// Baby class penalty
if (isBabyClass) make_per = Math.floor(make_per * 0.7);  // 30% reduction

// Clamp to 0-100%
make_per = Math.max(0, Math.min(10000, make_per));

// Final success chance: make_per / 100 = percentage
const successChance = make_per / 100;
```

### Example Success Rates

**Lv1 Weapon, Smith Lv3, no extras:**
- `5000 + 50*20 + 99*10 + 99*10 + 3*500 + 10*100 + 0 = 5000 + 1000 + 990 + 990 + 1500 + 1000 = 10480` -> capped at 100%

**Lv3 Weapon + Fire + 3 Stars, Smith Lv3:**
- `5000 + 50*20 + 99*10 + 99*10 + 3*500 + 10*100 + 5*100 + 0 - 2000 - 4500 - 2000`
- `= 5000 + 1000 + 990 + 990 + 1500 + 1000 + 500 - 2000 - 4500 - 2000 = 1480` -> 14.8%

### Forged Weapon Properties

When a weapon is forged, it gains:
1. **Crafter name** -- weapon displays "XXX's [Weapon Name]" (e.g., "Smith's Saber")
2. **Element** (if elemental stone used) -- Fire/Water/Wind/Earth Lv1
3. **Star Crumb bonuses** -- +5/+10/+40 Mastery ATK (piercing, ignores DEF, always hits)
4. **No card slots** -- forged weapons have 0 card slots (the slots are replaced by element/star crumbs)

**Star Crumb Mastery ATK Values:**

| Star Crumbs Used | Mastery ATK Bonus |
|-----------------|-------------------|
| 1 | +5 |
| 2 | +10 |
| 3 | +40 |

**Note:** The +40 for 3 stars is NOT a typo -- it is intentionally a large jump to reward the difficulty of forging with 3 stars.

### Ranked Blacksmith Bonus

The top 10 Blacksmiths on a server are "ranked" based on forging points:
- Ranked Blacksmith's forged weapons deal +10 Mastery ATK bonus (piercing, ignores DEF)
- Points earned: +1 (Lv1 weapon +10 refine), +10 (Lv3 weapon with 3 components), +25 (Lv2 weapon +10 refine), +1000 (Lv3 weapon +10 refine)

### Forgeable Weapons and Materials

#### Smith Dagger Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Knife | Lv1 | 1 Iron |
| Main Gauche | Lv1 | 50 Iron |
| Dirk | Lv2 | 17 Steel |
| Dagger | Lv2 | 30 Steel |
| Stiletto | Lv2 | 40 Steel |
| Gladius | Lv3 | 40 Steel, 4 Oridecon, 1 Sapphire |
| Damascus | Lv3 | 60 Steel, 4 Oridecon, 1 Zircon |

#### Smith Sword Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Sword | Lv1 | 2 Iron |
| Falchion | Lv1 | 30 Iron |
| Blade | Lv1 | 45 Iron, 25 Tooth of Bat |
| Rapier | Lv2 | 20 Steel |
| Scimitar | Lv2 | 35 Steel |
| Ring Pommel Saber | Lv2 | 40 Steel, 50 Wolf Claw |
| Saber | Lv3 | 5 Steel, 8 Oridecon, 1 Opal |
| Haedongeum | Lv3 | 10 Steel, 8 Oridecon, 1 Topaz |
| Tsurugi | Lv3 | 15 Steel, 8 Oridecon, 1 Garnet |
| Flamberge | Lv3 | 16 Oridecon, 1 Cursed Ruby |

#### Smith Two-Handed Sword Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Katana | Lv1 | 35 Iron, 15 Horrendous Mouth |
| Slayer | Lv2 | 25 Steel, 20 Decayed Nail |
| Bastard Sword | Lv2 | 45 Steel |
| Two-Handed Sword | Lv3 | 10 Steel, 12 Oridecon |
| Broad Sword | Lv3 | 20 Steel, 12 Oridecon |
| Claymore | Lv3 | 20 Steel, 16 Oridecon, 1 Cracked Diamond |

#### Smith Axe Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Axe | Lv1 | 10 Iron |
| Battle Axe | Lv1 | 110 Iron |
| Hammer | Lv2 | 30 Steel |
| Buster | Lv3 | 20 Steel, 4 Oridecon, 30 Orc's Fang |
| Two-Handed Axe | Lv3 | 10 Steel, 8 Oridecon, 1 Amethyst |

#### Smith Mace Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Club | Lv1 | 3 Iron |
| Mace | Lv1 | 30 Iron |
| Smasher | Lv2 | 20 Steel |
| Flail | Lv2 | 33 Steel |
| Chain | Lv2 | 45 Steel |
| Morning Star | Lv3 | 85 Steel, 1 2-Carat Diamond |

#### Smith Knucklebrace Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Knuckle Duster | Lv1 | 3 Iron |
| Brass Knuckle | Lv1 | 30 Iron |
| Waghnak | Lv2 | 20 Steel |
| Hora | Lv2 | 33 Steel |
| Fist | Lv2 | 40 Steel, 10 Cobaltite |
| Claw | Lv3 | 5 Steel, 8 Oridecon, 1 Opal |

#### Smith Spear Weapons

| Weapon | Level | Materials |
|--------|-------|-----------|
| Javelin | Lv1 | 3 Iron |
| Spear | Lv1 | 30 Iron |
| Pike | Lv2 | 20 Steel |
| Guisarme | Lv2 | 35 Steel, 5 Feather of Birds |
| Glaive | Lv2 | 40 Steel, 30 Tail of Steel Scorpion |
| Partizan | Lv3 | 5 Steel, 8 Oridecon, 1 Opal |
| Trident | Lv3 | 35 Steel, 12 Oridecon, 1 Aquamarine |

### Unforgeable Weapon Types

The following weapon types CANNOT be forged:
- Rods/Staves
- Bows
- Instruments
- Books
- Katars
- Level 4 weapons (any type)
- Guns/Firearms

---

## Refining System Architecture

### Overview

Refining (upgrading) increases a weapon's ATK or an armor's DEF. Blacksmiths get the Whitesmith skill "Upgrade Weapon" (rA: 477) which gives higher success rates than NPC refiners. This system is separate from forging.

### Refine Materials

| Equipment Type | Material | NPC Cost |
|---------------|----------|----------|
| Weapon Lv1 | 1 Phracon | 200z + 50z fee |
| Weapon Lv2 | 1 Emveretarcon | 1,000z + 200z fee |
| Weapon Lv3 | 1 Oridecon | Monster drop + 5,000z fee |
| Weapon Lv4 | 1 Oridecon | Monster drop + 20,000z fee |
| All Armor | 1 Elunium | Monster drop + 2,000z fee |

### Safe Limits

| Equipment | Safe Limit |
|-----------|-----------|
| Weapon Lv1 | +7 |
| Weapon Lv2 | +6 |
| Weapon Lv3 | +5 |
| Weapon Lv4 | +4 |
| Armor | +4 |

### Refine Success Rates (Normal Ores)

| Refine Level | Weapon Lv1 | Weapon Lv2 | Weapon Lv3 | Weapon Lv4 | Armor |
|-------------|-----------|-----------|-----------|-----------|-------|
| +1 | 100% | 100% | 100% | 100% | 100% |
| +2 | 100% | 100% | 100% | 100% | 100% |
| +3 | 100% | 100% | 100% | 100% | 100% |
| +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 19% | 19% | 19% | 9% | 9% |

### ATK/DEF Bonus per Refine Level

| Weapon Level | ATK per Refine | Over-Safe Bonus |
|-------------|---------------|-----------------|
| Level 1 | +2 ATK | +3 per level beyond safe |
| Level 2 | +3 ATK | +5 per level beyond safe |
| Level 3 | +5 ATK | +7 per level beyond safe |
| Level 4 | +7 ATK | +13 per level beyond safe |
| Armor | +0.66 DEF (~+1 per 1.5 levels) | N/A |

**Over-upgrade example (Lv3 weapon, safe +5):**
- +1 to +5: 5 * 5 = +25 ATK
- +6 to +10: 5 * 5 + 5 * 7 = +25 + 35 = +60 ATK total (standard + over-upgrade bonus)

### Failure Consequence

**Equipment is permanently destroyed.** All cards, enchantments, and refine levels are lost. The ore and zeny fee are consumed. There is NO downgrade -- only destruction.

---

## Cart System Architecture

### Overview

The cart system is shared between Merchant, Blacksmith, and Alchemist classes. It provides:
- Extra storage (100 item slots, 8000 max weight)
- Cart-based attack skills (Cart Revolution, Cart Termination)
- Movement speed penalty (offset by Pushcart skill level)
- Vending (player shops)

### Cart Properties

| Property | Value |
|----------|-------|
| Max Slots | 100 items |
| Max Weight | 8000 |
| Rental | From Kafra NPC (free or small fee) |
| Speed Penalty | `(50 + 5 * PushcartLevel)` % of normal speed |
| At Pushcart Lv10 | 100% normal speed (no penalty) |

### Cart Weight Impact on Skills

**Cart Revolution (Merchant):**
```
DamageATK% = 150 + floor(100 * CartWeight / 8000)
```
- Empty cart: 150% ATK
- Half cart (4000): 200% ATK
- Full cart (8000): 250% ATK

**Cart Termination (Whitesmith):**
```
DamageATK% = floor(CartWeight / DamageMod)
Where DamageMod = 16 - SkillLevel (Lv10: 6)
```
- Full cart (8000) at Lv10: `floor(8000 / 6)` = 1333% ATK

### Cart Storage Implementation

```js
// Player object:
player.hasCart = false;
player.cartItems = [];        // Array of cart inventory items
player.cartWeight = 0;        // Current cart weight
player.maxCartWeight = 8000;  // Fixed

// Socket events:
// cart:open       -> Show cart inventory
// cart:move_item  -> Move item between inventory <-> cart
// cart:data       -> Full cart contents
```

---

## New Systems Required

### System 1: Blacksmith Combat Buff System (Priority: HIGH)

Four new buffs need to be implemented:

| Buff | Type | Key Effect | Party? |
|------|------|------------|--------|
| `adrenaline_rush` | ASPD buff | +30% ASPD (caster), +20% (party) | Yes (Axe/Mace only) |
| `weapon_perfection` | Size bypass | 100% damage to all sizes | Yes |
| `power_thrust` | ATK buff | +5-25% ATK (caster), +5-15% (party) | Yes |
| `maximize_power` | Toggle | Max weapon variance, SP drain | No |

**Integration with existing buff system:**
- Use `applyBuff()` / `expireBuffs()` from `ro_buff_system.js`
- Add buff-specific modifiers to `getBuffStatModifiers()`:
  - `aspdMultiplier` (already exists for future use)
  - `noSizePenalty` (new flag)
  - `atkPercent` (new -- adds to `atkMultiplier`)
  - `maximizePower` (new flag)
  - `weaponBreakChance` (new -- checked after each attack)

**Integration with damage pipeline:**
- `noSizePenalty` -> Step 6 of physical damage (SIZE_PENALTY override)
- `atkPercent` -> Step 9 of physical damage (buff ATK modifier)
- `maximizePower` -> Step 5 of physical damage (skip variance roll)
- `aspdMultiplier` -> `calculateASPD()` in `ro_damage_formulas.js`

### System 2: Passive Skill Bonuses for Blacksmith (Priority: HIGH)

Add to `getPassiveSkillBonuses()`:

```js
// Weaponry Research (1204): +2 ATK/HIT per level, all weapons
const weapResLv = learned[1204] || 0;
if (weapResLv > 0) {
    bonuses.bonusATK += weapResLv * 2;
    bonuses.bonusHIT += weapResLv * 2;
}

// Skin Tempering (1205): +4% fire resist, +1% neutral resist per level
const skinTempLv = learned[1205] || 0;
if (skinTempLv > 0) {
    if (!bonuses.elementResist) bonuses.elementResist = {};
    bonuses.elementResist.fire = (bonuses.elementResist.fire || 0) + skinTempLv * 4;
    bonuses.elementResist.neutral = (bonuses.elementResist.neutral || 0) + skinTempLv;
}

// Hilt Binding (1207): +1 STR, +4 ATK, +10% duration bonus
const hiltLv = learned[1207] || 0;
if (hiltLv > 0) {
    bonuses.bonusSTR = (bonuses.bonusSTR || 0) + 1;
    bonuses.bonusATK += 4;
    bonuses.hiltBindingDurationBonus = true;
}
```

### System 3: Element Resistance in Damage Pipeline (Priority: MEDIUM)

Skin Tempering provides flat element damage reduction. This requires a new step in the physical and magical damage pipelines:

```js
// After element modifier lookup, apply element resistance:
const eleResist = (target.elementResist && target.elementResist[atkElement]) || 0;
if (eleResist > 0) {
    totalATK = Math.floor(totalATK * (100 - eleResist) / 100);
}
```

This system will also be used by:
- Pasana Card (Fire resist), Swordfish Card (Water resist), etc.
- Raydric Card (Neutral resist)

### System 4: Weapon Break System (Priority: LOW)

Required for Power Thrust weapon break risk and Whitesmith's Melt Down. Components:
- `player.weaponBroken` flag
- When broken: weapon ATK = 0, element = neutral, all card/refine bonuses removed
- Repair via Weapon Repair skill or NPC
- Visual indicator on client (broken weapon icon)

### System 5: Cart Inventory System (Priority: DEFERRED)

See Cart System Architecture section above. Required for:
- Cart Revolution damage scaling (currently uses flat 150%)
- Vending (player shops)
- Cart Termination (Whitesmith)
- Cart Boost (Whitesmith)

### System 6: Forging System (Priority: DEFERRED)

Major crafting system. See Forging System Architecture section above. Components:
- Forging UI (client)
- Recipe database (server)
- Success rate calculation
- Forged weapon generation with crafter name/element/stars
- Blacksmith ranking system

### System 7: Refining System (Priority: DEFERRED)

Upgrade equipment. See Refining System Architecture section above. Components:
- Refine NPC interactions
- Success rate tables
- Equipment destruction on failure
- Whitesmith Upgrade Weapon skill (higher rates)

---

## Whitesmith/Mastersmith Skills

### Transcendent class of Blacksmith. Inherits ALL Merchant + Blacksmith skills.

| ID (future) | Skill | rA ID | Max Level | Type | Key Mechanic |
|-------------|-------|-------|-----------|------|-------------|
| TBD | Cart Boost | 387 | 1 | Active/Self | +100% move speed with cart, 60s, SP 20 |
| TBD | High Speed Cart Ram (Cart Termination) | 485 | 10 | Offensive/Single | Cart weight / (16-Lv) ATK%, stun 5-50%, SP 15, Zeny 600-1500 |
| TBD | Maximum Over Thrust (Max Power-Thrust) | 486 | 5 | Active/Self | +20-100% ATK self-only, 60s, SP 15, Zeny 3000-5000, weapon break risk |
| TBD | Melt Down (Shattering Strike) | 384 | 10 | Active/Self | Chance to break enemy weapon/armor per attack, 15-60s |
| TBD | Upgrade Weapon (Weapon Refine) | 477 | 10 | Active/Friendly | Refine weapons with better success rate than NPC |

### Cart Boost (rA: 387)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 1 |
| SP Cost | 20 |
| Duration | 60 seconds |
| Prerequisites | Pushcart Lv5, Cart Revolution Lv1, Change Cart Lv1, Hilt Binding Lv1 |
| Required State | Cart must be equipped |

**Effect:** Increases movement speed to normal (removes cart speed penalty entirely) for 60 seconds. Also required to use Cart Termination.

### Cart Termination / High Speed Cart Ram (rA: 485)

| Property | Value |
|----------|-------|
| Type | Active, Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee (2 cells) |
| SP Cost | 15 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Mammonite Lv10, Hammer Fall Lv5, Cart Boost Lv1 |
| Required State | Cart Boost buff must be active |

**Damage Formula:**
```
DamageATK% = floor(CartWeight / DamageMod)
DamageMod = 16 - SkillLevel
```

| Level | Damage Mod | Stun% | Zeny Cost | Full Cart (8000) Damage |
|-------|-----------|-------|-----------|------------------------|
| 1 | 15 | 5% | 600z | 533% |
| 2 | 14 | 10% | 700z | 571% |
| 3 | 13 | 15% | 800z | 615% |
| 4 | 12 | 20% | 900z | 666% |
| 5 | 11 | 25% | 1000z | 727% |
| 6 | 10 | 30% | 1100z | 800% |
| 7 | 9 | 35% | 1200z | 888% |
| 8 | 8 | 40% | 1300z | 1000% |
| 9 | 7 | 45% | 1400z | 1142% |
| 10 | 6 | 50% | 1500z | 1333% |

**Zeny formula:** `500 + 100 * SkillLevel`

**Special mechanics:**
- Bypasses Guard, Cicada Skin Shed, and Parry
- Does NOT benefit from % card bonuses (Race/Size/Element cards) in pre-renewal
- Only flat ATK bonuses (from cards that add raw ATK) affect damage
- Stun bypasses Safety Wall
- Requires active Cart Boost status (cannot cast without it)

### Maximum Over Thrust / Maximum Power-Thrust (rA: 486)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self ONLY (not party) |
| SP Cost | 15 (all levels) |
| Duration | 60 seconds (all levels, some sources say 180s) |
| Prerequisites | Power Thrust Lv5 |

| Level | ATK% Bonus | Zeny Cost |
|-------|-----------|-----------|
| 1 | +20% | 3000z |
| 2 | +40% | 3500z |
| 3 | +60% | 4000z |
| 4 | +80% | 4500z |
| 5 | +100% | 5000z |

**ATK formula:** `20 * SkillLevel` %

**Weapon break risk:** Slight chance per attack (similar to Power Thrust). Self-only, does NOT affect party.

**Stacking:** Stacks multiplicatively with Power Thrust:
```
Total ATK% = BaseATK * (1 + PowerThrust%) * (1 + MaxOverThrust%)
Example at max: BaseATK * 1.25 * 2.0 = BaseATK * 2.50
```

### Melt Down / Shattering Strike (rA: 384)

| Property | Value |
|----------|-------|
| Type | Active, Supportive (self-buff) |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Skin Tempering Lv3, Hilt Binding Lv1, Weaponry Research Lv5, Power Thrust Lv3 |

| Level | SP | Duration(s) | Weapon Break% | Armor Break% |
|-------|-----|-----------|--------------|-------------|
| 1 | 50 | 15 | 1% | 0.7% |
| 2 | 50 | 20 | 2% | 1.4% |
| 3 | 60 | 25 | 3% | 2.1% |
| 4 | 60 | 30 | 4% | 2.8% |
| 5 | 70 | 35 | 5% | 3.5% |
| 6 | 70 | 40 | 6% | 4.2% |
| 7 | 80 | 45 | 7% | 4.9% |
| 8 | 80 | 50 | 8% | 5.6% |
| 9 | 90 | 55 | 9% | 6.3% |
| 10 | 90 | 60 | 10% | 7.0% |

**Against players (PvP):** Chance to break equipped weapon or armor.
**Against monsters:** Reduces ATK by 25% and DEF by 25% for ~5 seconds.
**Limitations:** Does NOT affect Boss/MVP monsters. Only affects one monster at a time.

### Upgrade Weapon / Weapon Refine (rA: 477)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 10 |
| SP Cost | 30 |
| Prerequisites | Weaponry Research Lv10 |

**Effect:** Allows the Whitesmith to refine weapons with a higher success rate than NPC refiners:
- At Job Level 50: equivalent to NPC rates
- At Job Level 60: +5% better than NPC
- At Job Level 70: +10% better than NPC

The skill level determines the maximum weapon level that can be refined:
- Lv1-2: Weapon Lv1 only
- Lv3-4: Up to Weapon Lv2
- Lv5-6: Up to Weapon Lv3
- Lv7+: All weapon levels

---

## Implementation Priority

### Phase 1: Prerequisite Fixes (Effort: ~15 min)

Fix all 9 prerequisite bugs in `ro_skill_data_2nd.js`. Fix Hammer Fall cooldown. Fix Power Thrust effectValue.

| Task | Skill ID | Fix |
|------|----------|-----|
| Add Hammer Fall Lv2 prereq | 1200 | `prerequisites: [{ skillId: 1206, level: 2 }]` |
| Add Weaponry Research Lv2 prereq | 1201 | `prerequisites: [{ skillId: 1200, level: 2 }, { skillId: 1204, level: 2 }]` |
| Fix Power Thrust prereq Lv3 not Lv5 | 1202 | `prerequisites: [{ skillId: 1200, level: 3 }]` |
| Fix Maximize Power prereq PT Lv2 not Lv5 | 1203 | `prerequisites: [{ skillId: 1202, level: 2 }, { skillId: 1201, level: 3 }]` |
| Remove Smith Dagger prereq | 1224 | `prerequisites: []` |
| Fix Smith Axe prereq to Smith Sword Lv2 | 1227 | `prerequisites: [{ skillId: 1225, level: 2 }]` |
| Fix Smith Mace prereq to Smith Knucklebrace Lv1 | 1228 | `prerequisites: [{ skillId: 1229, level: 1 }]` |
| Fix Smith Knucklebrace prereq to Smith Dagger Lv1 | 1229 | `prerequisites: [{ skillId: 1224, level: 1 }]` |
| Fix Smith Spear prereq to Smith Dagger Lv2 | 1230 | `prerequisites: [{ skillId: 1224, level: 2 }]` |
| Fix Research Oridecon prereq to ESC Lv1 | 1223 | `prerequisites: [{ skillId: 1222, level: 1 }]` |
| Remove Hammer Fall cooldown | 1206 | `cooldown: 0` |
| Fix Power Thrust effectValue | 1202 | `effectValue: 5*(i+1)` not `10+i*5` |

### Phase 2: Passive Skills (Effort: ~30 min)

Implement in `getPassiveSkillBonuses()`:
- Weaponry Research (+2 ATK/HIT per level)
- Skin Tempering (+4% fire, +1% neutral resist per level)
- Hilt Binding (+1 STR, +4 ATK, +10% duration flag)

Add element resistance to damage pipeline.

### Phase 3: Active Combat Skills (Effort: ~3 hours)

Implement handlers in `index.js` for:
1. **Hammer Fall** -- AoE stun (no damage), uses existing `applyStatusEffect()`
2. **Adrenaline Rush** -- Self + party ASPD buff, weapon restriction check
3. **Weapon Perfection** -- Self + party size bypass buff
4. **Power Thrust** -- Self + party ATK buff, weapon break chance
5. **Maximize Power** -- Toggle, SP drain in combat tick, max variance flag

Add buff integration:
- `aspdMultiplier` in ASPD calculation
- `noSizePenalty` in size penalty check
- `atkPercent` in ATK multiplier
- `maximizePower` in weapon variance
- `weaponBreakChance` in auto-attack tick

### Phase 4: VFX Configs (Effort: ~30 min)

Add VFX configs in `SkillVFXData.cpp` for:
- Hammer Fall: `AoEImpact` (ground slam)
- Adrenaline Rush: `SelfBuff` (red/orange aura)
- Weapon Perfection: `SelfBuff` (white/gold aura)
- Power Thrust: `SelfBuff` (orange/red aura)
- Maximize Power: `SelfBuff` (blue/gold aura, looping)

### Phase 5: Dubious Salesmanship Integration (Effort: ~5 min)

Check in Mammonite handler for Dubious Salesmanship (1211) and apply 10% zeny reduction.

### Phase 6: Cart System (Effort: LARGE -- deferred)

Full cart inventory, Kafra rental, cart weight tracking, Cart Revolution damage scaling.

### Phase 7: Forging System (Effort: VERY LARGE -- deferred)

Complete forging UI + server logic, recipe database, success rate calculation.

### Phase 8: Refining System (Effort: LARGE -- deferred)

Refine NPC, success rates, equipment destruction, Whitesmith Upgrade Weapon.

### Phase 9: Whitesmith Skills (Effort: LARGE -- deferred)

Cart Boost, Cart Termination, Maximum Over Thrust, Melt Down, Upgrade Weapon.

---

## Integration Points

### Existing Systems Affected

| System | Modification | Reason |
|--------|-------------|--------|
| `getPassiveSkillBonuses()` in `index.js` | Add Weaponry Research, Skin Tempering, Hilt Binding | 3 new passives |
| `getBuffStatModifiers()` in `ro_buff_system.js` | Add `noSizePenalty`, `atkPercent`, `maximizePower` | New buff effects |
| `calculatePhysicalDamage()` in `ro_damage_formulas.js` | Check `noSizePenalty` and `maximizePower` flags | Size bypass + max variance |
| `calculateASPD()` in `ro_damage_formulas.js` | Use `aspdMultiplier` from buff mods | Adrenaline Rush ASPD boost |
| Combat tick loop in `index.js` | SP drain for Maximize Power toggle | Toggle skill management |
| Auto-attack tick in `index.js` | Weapon break chance check (Power Thrust) | Equipment damage risk |
| `buildFullStatsPayload()` in `index.js` | Include new passive bonuses | Client stat display |
| `applyBuff()` in `ro_buff_system.js` | Hilt Binding duration extension | +10% duration for 3 buffs |
| Damage pipeline (both physical and magical) | Element resistance from Skin Tempering | Damage reduction |

### New Socket.io Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `forge:attempt` | Client -> Server | Request to forge a weapon |
| `forge:result` | Server -> Client | Forge success/failure + weapon data |
| `forge:recipe_list` | Server -> Client | Available recipes for player's skills |
| `refine:attempt` | Client -> Server | Request to refine equipment |
| `refine:result` | Server -> Client | Refine success/failure (destruction on fail) |
| `cart:open` | Client -> Server | Open cart inventory |
| `cart:data` | Server -> Client | Cart contents |
| `cart:move_item` | Client -> Server | Move item to/from cart |
| `equipment:broken` | Server -> Client | Weapon broke (Power Thrust) |
| `equipment:repaired` | Server -> Client | Weapon repaired |

---

## Constants and Data Tables

### Blacksmith-Specific Constants

```js
// Adrenaline Rush ASPD multipliers
const ADRENALINE_RUSH_ASPD_SELF = 1.3;     // 30% faster for caster
const ADRENALINE_RUSH_ASPD_PARTY = 1.2;    // 20% faster for party
const ADRENALINE_RUSH_WEAPONS = ['axe', 'one_hand_axe', 'two_hand_axe', 'mace'];

// Weapon Perfection
const WEAPON_PERFECTION_SIZE_BYPASS = true; // All sizes = 100%

// Power Thrust weapon break
const POWER_THRUST_BREAK_CHANCE = 0.001;    // 0.1% per attack

// Maximize Power
const MAXIMIZE_POWER_ACTIVATION_SP = 10;    // One-time SP cost
const MAXIMIZE_POWER_DRAIN_INTERVALS = [1000, 2000, 3000, 4000, 5000]; // ms per 1 SP drain

// Hilt Binding
const HILT_BINDING_DURATION_BONUS = 1.1;    // +10% duration
const HILT_BINDING_AFFECTED_BUFFS = ['adrenaline_rush', 'weapon_perfection', 'power_thrust'];

// Skin Tempering
const SKIN_TEMPERING_FIRE_PER_LV = 4;      // +4% fire resist per level
const SKIN_TEMPERING_NEUTRAL_PER_LV = 1;   // +1% neutral resist per level

// Hammer Fall
const HAMMER_FALL_AOE_RADIUS = 250;         // 5x5 cells in UE units
const HAMMER_FALL_STUN_DURATION = 5000;     // 5 seconds base

// Forging
const FORGE_BASE_RATE = 5000;              // 50% base (out of 10000)
const FORGE_JOB_LEVEL_BONUS = 20;          // +0.2% per job level
const FORGE_DEX_BONUS = 10;                // +0.1% per DEX
const FORGE_LUK_BONUS = 10;                // +0.1% per LUK
const FORGE_SMITH_SKILL_BONUS = 500;       // +5% per Smith skill level
const FORGE_WEAPONRY_RESEARCH_BONUS = 100; // +1% per WR level
const FORGE_ORIDECON_RESEARCH_BONUS = 100; // +1% per OR level (Lv3 weapons only)
const FORGE_ELEMENT_PENALTY = 2000;        // -20% for elemental stone
const FORGE_STAR_CRUMB_PENALTY = 1500;     // -15% per star crumb
const FORGE_WEAPON_LEVEL_PENALTY = 1000;   // -10% per weapon level above 1

// Anvil bonuses
const ANVIL_BONUS = {
    'anvil': 0,
    'oridecon_anvil': 300,
    'golden_anvil': 500,
    'emperium_anvil': 1000
};

// Star Crumb mastery ATK
const STAR_CRUMB_MASTERY_ATK = [0, 5, 10, 40]; // index = number of crumbs

// Refine success rates (out of 100)
const REFINE_RATES = {
    weapon_lv1: [100, 100, 100, 100, 100, 100, 100, 60, 40, 19],
    weapon_lv2: [100, 100, 100, 100, 100, 100, 60, 40, 20, 19],
    weapon_lv3: [100, 100, 100, 100, 100, 60, 50, 20, 20, 19],
    weapon_lv4: [100, 100, 100, 100, 60, 40, 40, 20, 20, 9],
    armor:      [100, 100, 100, 100, 60, 40, 40, 20, 20, 9]
};

// Refine ATK bonuses
const REFINE_ATK_PER_LEVEL = { 1: 2, 2: 3, 3: 5, 4: 7 };
const REFINE_OVER_SAFE_BONUS = { 1: 3, 2: 5, 3: 7, 4: 13 };
const REFINE_SAFE_LIMIT = { 1: 7, 2: 6, 3: 5, 4: 4, armor: 4 };
```

---

## Sources

- [Blacksmith - iRO Wiki](https://irowiki.org/wiki/Blacksmith)
- [Blacksmith - iRO Wiki Classic](https://irowiki.org/classic/Blacksmith)
- [Blacksmith Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=10)
- [Blacksmith Skill Simulator - RateMyServer](https://ratemyserver.net/skill_sim.php?jid=10)
- [Adrenaline Rush - iRO Wiki](https://irowiki.org/wiki/Adrenaline_Rush)
- [Adrenaline Rush - iRO Wiki Classic](https://irowiki.org/classic/Adrenaline_Rush)
- [Adrenaline Rush - rAthena Pre-Re](https://pre.pservero.com/skill/BS_ADRENALINE)
- [Power-Thrust - iRO Wiki](https://irowiki.org/wiki/Power-Thrust)
- [Power-Thrust - rAthena Pre-Re](https://pre.pservero.com/skill/BS_OVERTHRUST)
- [Weapon Perfection - iRO Wiki](https://irowiki.org/wiki/Weapon_Perfection)
- [Weapon Perfection - rAthena Pre-Re](https://pre.pservero.com/skill/BS_WEAPONPERFECT)
- [Maximize Power - iRO Wiki](https://irowiki.org/wiki/Maximize_Power)
- [Maximize Power - rAthena Pre-Re](https://pre.pservero.com/skill/BS_MAXIMIZE)
- [Hammer Fall - iRO Wiki](https://irowiki.org/wiki/Hammer_Fall)
- [Skin Tempering - iRO Wiki](https://irowiki.org/wiki/Skin_Tempering)
- [Weaponry Research - iRO Wiki](https://irowiki.org/wiki/Weaponry_Research)
- [Forging - iRO Wiki](https://irowiki.org/wiki/Forging)
- [Forging - iRO Wiki Classic](https://irowiki.org/classic/Forging)
- [Refinement System - iRO Wiki Classic](https://irowiki.org/classic/Refinement_System)
- [Refine Success Rates - RateMyServer](https://ratemyserver.net/index.php?page=misc_table_refine)
- [Mastersmith - iRO Wiki](https://irowiki.org/wiki/Mastersmith)
- [Whitesmith Skills - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=4011)
- [High Speed Cart Ram - iRO Wiki](https://irowiki.org/wiki/High_Speed_Cart_Ram)
- [Cart Revolution - iRO Wiki](https://irowiki.org/wiki/Cart_Revolution)
- [rAthena Pre-Renewal Skill DB (GitHub)](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [RO Forge Calculator - RateMyServer](https://ratemyserver.net/forge_calc.php)
- [Blacksmithing - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Blacksmithing)
- [Merchant Skills Audit (existing)](docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md)
