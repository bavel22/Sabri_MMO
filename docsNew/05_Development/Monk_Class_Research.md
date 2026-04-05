# Monk Class Research -- RO Classic (Pre-Renewal) Comprehensive Implementation Document

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Monk_Skills_Audit](Monk_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Base Class:** Acolyte (inherits all Acolyte skills)
**Class Progression:** Novice -> Acolyte -> Monk -> Champion (transcendent)
**Scope:** 16 Monk skills (14 standard + 2 quest), Spirit Sphere system, Combo system

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Spirit Sphere System](#spirit-sphere-system)
3. [Combo System Architecture](#combo-system-architecture)
4. [Complete Skill List](#complete-skill-list)
5. [Skill Tree and Prerequisites](#skill-tree-and-prerequisites)
6. [New Systems Required](#new-systems-required)
7. [Existing Skill Definition Corrections](#existing-skill-definition-corrections)
8. [Implementation Priority](#implementation-priority)
9. [Integration Points](#integration-points)
10. [Monk-Specific Constants](#monk-specific-constants)
11. [Sources](#sources)

---

## Class Overview

The Monk is a melee DPS/burst class that branches from Acolyte. Unlike the Priest (the other Acolyte branch), Monks focus on physical combat using fists, knuckle weapons, and spirit spheres. They are defined by three core systems:

1. **Spirit Spheres** -- Floating holy orbs that provide +3 ATK each and are consumed by many skills
2. **Combo System** -- Chain skills in sequence (Triple Attack -> Chain Combo -> Combo Finish -> Asura Strike) within tight timing windows
3. **Burst Damage** -- Asura Strike consumes all SP for a single devastating hit

**Class Identity:** High single-target burst damage. Can either play as an AGI combo monk (fast auto-attacks with Triple Attack procs leading into combos) or as a STR/INT burst monk (maximizing SP pool for Asura Strike damage).

**Weapon Types:** Bare fists, Knuckle/Claw weapons, Maces, Staves (inherits Acolyte weapon proficiency)

**ASPD Base Delays (already in `ro_exp_tables.js`):**
```
monk: { bare_hand: 36, mace: 60, staff: 62, knuckle: 42 }
```

**Size Penalties (already in `ro_damage_formulas.js`):**
- Bare Hand: 100% / 100% / 100% (Small / Medium / Large)
- Knuckle: 100% / 75% / 50%
- Mace: 75% / 100% / 100%

**Job Bonuses (per 50 job levels):** +8 STR, +7 AGI, +6 VIT, +2 INT, +4 DEX, +3 LUK

---

## Spirit Sphere System

### Overview

Spirit Spheres are the Monk's core resource. They are floating holy orbs that orbit the character, providing passive ATK bonuses and serving as fuel for many Monk skills.

### Mechanics

| Property | Value |
|----------|-------|
| Max Spheres | 5 (Monk), up to 10 (Champion with Tiger Cannon) |
| ATK Bonus | +3 per sphere (flat, added to weapon ATK) |
| ATK Type | Ignores enemy DEF and FLEE (mastery-like flat bonus) |
| Element | Holy (not affected by weapon element cards) |
| Duration | 10 minutes (600,000 ms) per summon batch |
| Persistence | Survives death, lost on disconnect/relog |
| Summoning | Summon Spirit Sphere skill -- 1 sphere per cast |
| Consumption | Various skills consume 1 or 5 spheres |

### Server-Side Data Structure

```js
// On player object:
player.spiritSpheres = 0;          // Current sphere count (0-5)
player.maxSpiritSpheres = 5;       // Max allowed (5 for Monk, may increase for Champion)
player.sphereExpireTime = 0;       // Timestamp when spheres expire (10 min from last summon)
```

### Sphere ATK Application

Spirit Sphere ATK (+3 per sphere) is a mastery-type bonus:
- Added AFTER all multipliers (like Sword Mastery ATK)
- NOT affected by element modifiers
- NOT affected by size penalties
- NOT affected by card multipliers
- IS affected by target FLEE (contrary to some sources, spheres are just flat ATK in practice)

In `getPassiveSkillBonuses()` or as a runtime modifier, add `player.spiritSpheres * 3` to the flat ATK bonus.

### Sphere Socket Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `sphere:update` | Server -> Client | `{ characterId, sphereCount, maxSpheres }` | Sphere count changed |
| `sphere:update` | Server -> Zone | `{ characterId, sphereCount }` | Others see sphere count (for VFX) |

### Client-Side VFX

Spirit spheres should be rendered as small glowing holy orbs orbiting the character at waist height. The number of visible orbs matches `sphereCount`. When a sphere is consumed, one orb disappears. When summoned, a new orb appears.

---

## Combo System Architecture

### Overview

The Monk combo system allows chaining specific skills in sequence, each skill only usable during the after-delay of the previous skill. This creates a timing-based gameplay loop.

### Combo Chain

```
Auto-Attack (Triple Attack procs at 20-29% chance)
    |
    v
Triple Attack (passive, 3 hits, 120-300% ATK)
    |-- Combo window opens --
    v
Chain Combo (4 hits, 200-400% ATK, 11-15 SP)
    |-- Combo window opens --
    v
Combo Finish (1 hit, 300-540% ATK, 11-15 SP, 1 sphere, 5x5 AoE splash)
    |-- Combo window opens (if Fury active + 4+ spheres) --
    v
Asura Strike (1 hit, massive damage, consumes ALL SP + all spheres)
```

### Combo Timing Window

**Combo Delay Formula:**
```
comboWindow = 1.3 - (AGI * 0.004) - (DEX * 0.002)   // seconds
```

| AGI | DEX | Window (seconds) |
|-----|-----|-----------------|
| 1 | 1 | 1.294s |
| 50 | 30 | 1.040s |
| 80 | 50 | 0.880s |
| 99 | 80 | 0.744s |
| 99 | 99 | 0.706s |

The window opens immediately when the previous combo skill executes and closes after `comboWindow` seconds. The player must press the next combo skill's hotkey during this window.

### Server-Side Combo State

```js
// Per-player combo tracking (stored on player object or in a Map)
player.comboState = {
    active: false,              // Is a combo chain in progress?
    lastSkillId: null,          // ID of the last combo skill that executed
    windowExpires: 0,           // Date.now() + comboWindowMs -- when the window closes
    targetId: null,             // The target being comboed (carried forward)
    isEnemy: true               // Whether target is an enemy
};
```

### Combo Validation Rules

1. **Triple Attack -> Chain Combo**: Chain Combo can ONLY be used during the combo window after Triple Attack procs. No other way to activate it (except during Blade Stop Lv4+).
2. **Chain Combo -> Combo Finish**: Combo Finish can ONLY be used during the combo window after Chain Combo. No other way to activate it (except during Blade Stop).
3. **Combo Finish -> Asura Strike**: Asura Strike can be chained after Combo Finish ONLY if the player has Fury status active AND at least 1 spirit sphere remaining (the Combo Finish consumed 1, so need 5 before starting combo, leaving 4). When used via combo, Asura Strike has NO cast time.
4. **Blade Stop Lv4**: Allows Chain Combo during Root lock (bypasses Triple Attack requirement).
5. **Blade Stop Lv5**: Allows Asura Strike during Root lock (with Fury + spheres).

### Combo Skill Properties

All combo skills:
- Have **0 cast time** (instant activation within combo window)
- Use **weapon element** (not forced element)
- Are **physical damage** (use ATK pipeline)
- **Cannot miss** (combo hits always connect -- they follow the initial attack)
- Target is **inherited** from the initial auto-attack / Triple Attack target

### Server-Side Combo Flow

```
1. Auto-attack tick fires
2. Triple Attack check: roll 20-29% chance (based on skill level)
3. If TA procs:
   a. Calculate TA damage (3 hits, ATK * effectValue%)
   b. Broadcast skill:effect_damage for Triple Attack
   c. Set player.comboState = { active: true, lastSkillId: 1603, windowExpires: now + comboWindowMs, targetId, isEnemy }
   d. Broadcast skill:combo_window { characterId, nextSkillId: 1610, windowMs }
4. Player sends skill:use { skillId: 1610 (Chain Combo) }
5. Server validates:
   - Is combo active? Is lastSkillId === 1603? Is window still open?
   - Does player have Chain Combo learned?
   - Does player have enough SP?
6. Execute Chain Combo (4 hits), deduct SP
7. Set comboState.lastSkillId = 1610, reset windowExpires
8. Broadcast skill:combo_window { characterId, nextSkillId: 1613, windowMs }
9. Player sends skill:use { skillId: 1613 (Combo Finish) }
10. Validate, execute, deduct 1 sphere + SP
11. If Fury active + spheres >= 1:
    Set comboState for Asura Strike window
12. Player sends skill:use { skillId: 1605 (Asura Strike) }
13. Execute Asura with NO cast time (combo bypass)
```

### Client-Side Combo UI

When a combo window opens, the next skill in the chain should flash/pulse on the hotbar to indicate it is available. The `skill:combo_window` event tells the client which skill to highlight and for how long.

---

## Complete Skill List

### Skill ID Allocation: 1600-1615

All Monk skills use IDs 1600-1615 (already defined in `ro_skill_data_2nd.js`).

---

### 1. IRON FISTS (ID 1600) -- Passive

| Property | Value |
|----------|-------|
| rAthena ID | 259 (MO_IRONHAND) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Demon Bane Lv10 (413), Divine Protection Lv10 (401) |
| Effect | +3 ATK per level with Knuckle weapons or bare hands |
| Tree Position | Row 0, Col 0 |

**ATK Bonus Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK | +3 | +6 | +9 | +12 | +15 | +18 | +21 | +24 | +27 | +30 |

**Implementation Notes:**
- Identical to Sword Mastery in implementation pattern
- Applies to both bare hands AND knuckle weapons
- Added as mastery ATK (flat, after multipliers)
- Added to `getPassiveSkillBonuses()` as `ironFistsATK` or reuse `bonusATK` with weapon type check
- Weapon type check: `weaponType === 'knuckle' || weaponType === 'fist' || weaponType === 'bare_hand'`

**Current Definition Status:** CORRECT -- `effectValue: (i+1)*3` gives +3 to +30.

---

### 2. SUMMON SPIRIT SPHERE (ID 1601) -- Active, Self

| Property | Value |
|----------|-------|
| rAthena ID | 261 (MO_CALLSPIRITS) |
| Type | Active, Self |
| Max Level | 5 |
| Prerequisites | Iron Fists Lv2 (1600) |
| SP Cost | 8 (all levels) |
| Cast Time | 1000ms (uninterruptible) |
| After-Cast Delay | 0 |
| Duration | 10 minutes (600,000 ms) |
| Tree Position | Row 0, Col 1 |

**Max Spheres Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Max Spheres | 1 | 2 | 3 | 4 | 5 |

**Mechanics:**
- Each cast summons exactly 1 sphere (up to the level's max)
- If already at max for skill level, no additional sphere is gained (SP still consumed)
- Each sphere provides +3 ATK (holy, ignores DEF)
- Sphere ATK is NOT affected by cards or elements
- Spheres persist through death but are lost on disconnect
- Casting resets the 10-minute timer for ALL spheres (not per-sphere timers)

**Server Handler:**
```js
if (skill.name === 'summon_spirit_sphere') {
    const maxSpheres = learnedLevel; // 1-5
    if (player.spiritSpheres >= maxSpheres) {
        // Reset timer only (refreshes duration)
        player.sphereExpireTime = Date.now() + 600000;
    } else {
        player.spiritSpheres = Math.min(maxSpheres, player.spiritSpheres + 1);
        player.sphereExpireTime = Date.now() + 600000;
    }
    // Deduct SP, apply delays
    // Broadcast sphere:update to zone
}
```

**Current Definition Status:** CORRECT -- `spCost: 8, castTime: 1000, effectValue: i+1 (max spheres), duration: 600000`.

---

### 3. INVESTIGATE (ID 1602) -- Active, Single Target

| Property | Value |
|----------|-------|
| rAthena ID | 266 (MO_INVESTIGATE) |
| Type | Active, Offensive, Physical |
| Max Level | 5 |
| Prerequisites | Summon Spirit Sphere Lv5 (1601) |
| Target | Single Enemy |
| Range | 2 cells (100 UE units -- melee range) |
| Cast Time | 1000ms (interruptible) |
| After-Cast Delay | 500ms |
| Sphere Cost | 1 per cast |
| Element | Always Neutral (ignores weapon element) |
| Hit Type | Always hits (ignores FLEE and Perfect Dodge) |
| Tree Position | Row 1, Col 0 |

**Pre-Renewal Damage Formula:**
```
Damage = ATK * (1 + 0.75 * SkillLv) * (EnemyHardDEF + EnemyVIT) / 50
```

Equivalent formulas (mathematically identical):
- iRO Wiki: `(200% + 150% * level) * (DEF + VITDEF) / 100`
- jRO Wiki: `(100% + 75% * level) * (DEF + VITDEF) / 50`

**Key mechanic: DEF is NOT subtracted from damage -- it MULTIPLIES damage.** The higher the target's DEF, the more damage Investigate deals. This makes it excellent against heavily armored targets (high DEF monsters) but weak against low-DEF targets.

**Per-Level Data:**

| Level | ATK Multiplier | DEF Multiplier Formula | SP Cost |
|-------|---------------|----------------------|---------|
| 1 | 175% | (DEF + VIT) / 50 | 10 |
| 2 | 250% | (DEF + VIT) / 50 | 14 |
| 3 | 325% | (DEF + VIT) / 50 | 17 |
| 4 | 400% | (DEF + VIT) / 50 | 19 |
| 5 | 475% | (DEF + VIT) / 50 | 20 |

**Special Mechanics:**
- Bypasses normal DEF reduction pipeline -- DEF is used as a damage multiplier instead
- Always hits (skip HIT/FLEE check entirely, skip Perfect Dodge check)
- Always Neutral element regardless of weapon element
- Size penalty still applies (knuckle: 100%/75%/50% vs S/M/L)
- Card bonuses still apply (race/element/size cards)
- If enemy has 0 DEF + 0 VIT, damage is 0 (skill effectively misses)
- After-cast delay can be cancelled by immediately casting Summon Spirit Sphere

**Server Handler Notes:**
This requires a CUSTOM damage calculation -- cannot use `calculatePhysicalDamage()` directly because it inverts the DEF pipeline. Need a dedicated function:
```js
function calculateInvestigateDamage(attacker, target, skillLevel) {
    // 1. Get StatusATK + WeaponATK (normal ATK calculation)
    // 2. Apply skill multiplier: ATK * (1 + 0.75 * skillLevel)
    // 3. Apply DEF multiplier: *= (targetHardDEF + targetSoftDEF) / 50
    // 4. Apply card bonuses (race, element, size -- multiplicative)
    // 5. Apply size penalty
    // 6. Floor to min 1 (or 0 if target DEF+VIT = 0)
    // NOTE: Do NOT subtract DEF or apply DEF reduction!
}
```

**Current Definition Corrections Needed:**
- `range: 150` should stay (2 cells = ~100 UE, but 150 with tolerance is fine for melee)
- `castTime: 500` should be `castTime: 1000` (1 second per all sources)
- `cooldown: 500` should be `afterCastDelay: 500, cooldown: 0` (0.5s ACD, not per-skill CD)
- `effectValue: 150+i*50` giving 150,200,250,300,350 -- this does NOT match the canonical formula. The canonical ATK% is 175, 250, 325, 400, 475. Need to use manual SP costs too: 10, 14, 17, 19, 20.
- `spCost: 10+i*3` gives 10,13,16,19,22 -- should be 10,14,17,19,20

---

### 4. TRIPLE ATTACK (ID 1603) -- Passive

| Property | Value |
|----------|-------|
| rAthena ID | 263 (MO_TRIPLEATTACK) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Dodge Lv5 (1608) |
| Tree Position | Row 1, Col 1 |

**IMPORTANT:** The current skill definition lists prerequisites as `Iron Fists Lv5 (1600)` but the canonical prerequisite is `Dodge Lv5 (1608)`. This needs correction.

**Mechanics:**
- Chance to trigger on each auto-attack (like Double Attack for Thieves)
- When triggered, the auto-attack becomes 3 hits at an increased ATK multiplier
- Higher skill levels DECREASE the proc chance but INCREASE the damage
- Counts as a single strike for card bonus purposes (bonuses applied once to total)
- Uses weapon element
- Can be the starting point for the combo chain (opens Chain Combo window)
- Does NOT consume spirit spheres
- No SP cost (passive)

**Per-Level Data:**

| Level | Proc Chance | Total ATK% | Per-Hit ATK% |
|-------|------------|-----------|-------------|
| 1 | 29% | 120% | 40% each |
| 2 | 28% | 140% | ~47% each |
| 3 | 27% | 160% | ~53% each |
| 4 | 26% | 180% | 60% each |
| 5 | 25% | 200% | ~67% each |
| 6 | 24% | 220% | ~73% each |
| 7 | 23% | 240% | 80% each |
| 8 | 22% | 260% | ~87% each |
| 9 | 21% | 280% | ~93% each |
| 10 | 20% | 300% | 100% each |

**Proc Chance Formula:** `30 - level` %
**ATK% Formula:** `100 + 20 * level` %

**Combo Delay (time window to chain into Chain Combo):**
```
comboDelay = 1.3 - (AGI * 0.004) - (DEX * 0.002)   // seconds
```

**Implementation in Auto-Attack Tick:**
```js
// In processEnemyAutoAttack() or processPlayerAutoAttack():
const tripleAttackLv = player.learnedSkills[1603] || 0;
if (tripleAttackLv > 0) {
    const tripleChance = 30 - tripleAttackLv; // 29% at Lv1 to 20% at Lv10
    if (Math.random() * 100 < tripleChance) {
        // Triple Attack triggered!
        const atkMultiplier = 100 + 20 * tripleAttackLv; // 120-300%
        // Calculate damage with skill multiplier (3 hits, total = atkMultiplier%)
        // Broadcast 3 hit events with stagger
        // Open combo window for Chain Combo
        player.comboState = {
            active: true,
            lastSkillId: 1603,
            windowExpires: Date.now() + comboWindowMs,
            targetId, isEnemy
        };
        // Skip normal auto-attack damage
        return;
    }
}
```

**Interaction with Double Attack (Thief passive):**
- If a character somehow has both (e.g., Soul Linker buff), Double Attack takes priority
- Triple Attack replaces Double Attack check for Monks

**Current Definition Corrections Needed:**
- `prerequisites: [{ skillId: 1600, level: 5 }]` should be `[{ skillId: 1608, level: 5 }]` (Dodge Lv5, not Iron Fists Lv5)
- `effectValue: 120+i*20` is CORRECT (120, 140, 160, ... 300)

---

### 5. FINGER OFFENSIVE (ID 1604) -- Active, Single Target Ranged

| Property | Value |
|----------|-------|
| rAthena ID | 267 (MO_FINGEROFFENSIVE) |
| Type | Active, Offensive, Physical |
| Max Level | 5 |
| Prerequisites | Investigate Lv3 (1602) |
| Target | Single Enemy |
| Range | 9 cells (450 UE units) |
| Cast Time | (1 + spheres_consumed) seconds -- variable based on spheres used |
| After-Cast Delay | 500ms |
| SP Cost | 10 (all levels) |
| Element | Weapon element |
| Hit Type | Multi-hit (1 hit per sphere consumed) |
| Tree Position | Row 2, Col 0 |

**Per-Level Data:**

| Level | ATK% Per Sphere | Max Spheres | Cast Time (max spheres) | Walk Delay |
|-------|----------------|------------|------------------------|-----------|
| 1 | 150% | 1 | 2.0s | 0.2s |
| 2 | 200% | 2 | 3.0s | 0.4s |
| 3 | 250% | 3 | 4.0s | 0.6s |
| 4 | 300% | 4 | 5.0s | 0.8s |
| 5 | 350% | 5 | 6.0s | 1.0s |

**ATK% Formula:** `100 + 50 * level` per sphere
**Cast Time Formula:** `(1 + spheres_consumed) * 1000` ms (reduced by DEX)

**Key Mechanics:**
- Consumes spheres equal to skill level (or all remaining if fewer)
- Each sphere becomes 1 hit at the skill's ATK% -- each sphere does FULL damage
- Example: Lv5 with 5 spheres = 5 hits at 350% ATK each = 1750% total
- Example: Lv5 with only 2 spheres = 2 hits at 350% ATK each = 700% total (cast time = 3s)
- Cast time scales with ACTUAL spheres consumed, not max level
- Walk delay prevents immediate movement after skill
- Ranged physical attack (9 cell range)
- Uses weapon element
- Size penalty applies

**Current Definition Corrections Needed:**
- `spCost: 10+i*3` gives 10,13,16,19,22 -- should be flat `spCost: 10` (all levels)
- `castTime: 1000` -- needs dynamic calculation based on spheres consumed. Store base as 1000 and add `spheres * 1000` at runtime
- `cooldown: 1000` should be `afterCastDelay: 500, cooldown: 0`
- `effectValue: 150+i*50` is CORRECT (150, 200, 250, 300, 350)

---

### 6. ASURA STRIKE (ID 1605) -- Active, Single Target

| Property | Value |
|----------|-------|
| rAthena ID | 271 (MO_EXTREMITYFIST) |
| Type | Active, Offensive, Physical |
| Max Level | 5 |
| Prerequisites | Finger Offensive Lv3 (1604), Critical Explosion Lv3 (1611) |
| Target | Single Enemy |
| Range | 2 cells (melee, ~100-150 UE units) |
| Sphere Cost | All 5 spheres consumed |
| SP Cost | ALL remaining SP consumed (SP contributes to damage) |
| Element | Always Neutral (ignores weapon element) |
| Hit Type | Always hits (ignores FLEE). Bypasses DEF. |
| Required State | Fury (Critical Explosion) must be active |
| Tree Position | Row 3, Col 0 |

**THE CORE DAMAGE FORMULA (Pre-Renewal):**
```
Damage = (WeaponATK + StatusATK) * (8 + RemainingHP / 10) + 250 + (150 * SkillLevel)
```

**Simplified as rAthena source shows:**
```
skillratio = 100 * (7 + SP / 10)     // The ratio applied to ATK
// Then add flat bonus: + 250 + 150 * skillLevel
```

Wait -- the formula from multiple canonical sources is:
```
Damage = [(Weapon ATK + Base ATK) * (8 + SP/10) + 250 + (150 * SkillLv)] * Card Effects
```

Where:
- `Weapon ATK + Base ATK` = StatusATK + WeaponATK (standard ATK calculation)
- `SP` = Remaining SP at time of cast (ALL of it is consumed)
- `250 + 150 * SkillLv` = Flat bonus: 400, 550, 700, 850, 1000

**Per-Level Data:**

| Level | Flat Bonus | Cast Time | After-Cast Delay | SP Cost |
|-------|-----------|-----------|-----------------|---------|
| 1 | +400 | 4000ms | 3000ms | ALL |
| 2 | +550 | 3500ms | 2500ms | ALL |
| 3 | +700 | 3000ms | 2000ms | ALL |
| 4 | +850 | 2500ms | 1500ms | ALL |
| 5 | +1000 | 2000ms | 1000ms | ALL |

**Cast Time Formula:** `4500 - 500 * level` ms (before DEX reduction)
**After-Cast Delay Formula:** `3500 - 500 * level` ms
**Flat Bonus Formula:** `250 + 150 * level`

**Special Mechanics:**

1. **SP Consumption:** ALL remaining SP is consumed. The higher your SP at cast time, the more damage. This is the defining mechanic of Asura Strike.

2. **DEF Bypass:** Asura Strike bypasses the target's DEF entirely (both hard and soft DEF). However, card-based damage reductions (Thara Frog Card, etc.) still apply.

3. **Always Hits:** Ignores FLEE and Perfect Dodge. Cannot miss.

4. **Fury Requirement:** Critical Explosion (Fury) must be active. Using Asura Strike consumes the Fury status.

5. **Sphere Requirement:** All 5 spheres consumed on cast.

6. **Post-Cast SP Lockout:** After Asura Strike executes, the player CANNOT regenerate SP naturally for 5 minutes (300,000 ms). Spiritual Cadence (sitting regen) bypasses this lockout. Items that restore SP still work.

7. **Combo Usage:** When used after Combo Finish in a combo chain, Asura Strike has NO cast time (instant). Requires at least 1 sphere remaining (Combo Finish consumed 1, so need 5 before combo = 4 remaining). Fury must still be active.

8. **Blade Stop Usage:** When used during Root (Blade Stop) Lv5, Asura Strike has NO cast time. Requires Fury + at least 1 sphere.

9. **Mastery ATK Excluded:** Weapon mastery ATK is NOT included in the calculation.

10. **Not affected by Weapon Element:** Damage is always Neutral regardless of weapon element or endow buffs.

**Damage Example (1000 SP, 200 ATK, Lv5):**
```
Damage = 200 * (8 + 1000/10) + 1000
       = 200 * 108 + 1000
       = 21600 + 1000
       = 22600 (before card multipliers)
```

**Server Handler Sketch:**
```js
if (skill.name === 'asura_strike') {
    // Validate: Fury active, 5 spheres (or combo/root bypass)
    if (!hasBuff(player, 'critical_explosion') && !isComboBypass) {
        socket.emit('skill:error', { message: 'Fury state required' });
        return;
    }
    if (!isComboBypass && player.spiritSpheres < 5) {
        socket.emit('skill:error', { message: 'Need 5 Spirit Spheres' });
        return;
    }
    // Calculate damage
    const totalATK = getStatusATK(player) + getWeaponATK(player);
    const remainingSP = player.mana;
    const flatBonus = 250 + 150 * learnedLevel;
    let damage = Math.floor(totalATK * (8 + Math.floor(remainingSP / 10))) + flatBonus;
    // Apply card bonuses (race/element/size -- multiplicative)
    damage = applyCardBonuses(damage, player.cardMods, target);
    // Apply element modifier (Neutral vs target element)
    damage = Math.floor(damage * getElementModifier('neutral', targetElement, targetEleLv) / 100);
    // Do NOT apply DEF reduction!
    // Consume ALL SP
    player.mana = 0;
    // Consume all spheres
    player.spiritSpheres = 0;
    // Remove Fury buff
    removeBuff(player, 'critical_explosion');
    // Apply SP regen lockout (5 minutes)
    player.asuraRegenLockout = Date.now() + 300000;
    // Apply damage to target
    target.health = Math.max(0, target.health - damage);
    // Broadcast, death check, etc.
}
```

**Current Definition Corrections Needed:**
- `effectValue: 250+i*150` gives 250,400,550,700,850 -- should be 400,550,700,850,1000. Formula should be `250 + (i+1) * 150` = 400, 550, 700, 850, 1000
- `spCost: 0` -- CORRECT (SP is consumed dynamically, not a fixed cost)
- `castTime: 4000-i*500` gives 4000,3500,3000,2500,2000 -- CORRECT
- `cooldown: 3000` -- this should be `afterCastDelay` scaling per level, not fixed cooldown. ACD = 3000,2500,2000,1500,1000

---

### 7. SPIRITS RECOVERY (ID 1606) -- Passive

| Property | Value |
|----------|-------|
| rAthena ID | 260 (MO_SPIRITSRECOVERY) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Blade Stop Lv2 (1609) |
| Tree Position | Row 0, Col 2 |

**IMPORTANT:** Current definition has `prerequisites: [{ skillId: 1601, level: 2 }]` (Summon Spirit Sphere Lv2). Canonical prerequisite is `Blade Stop Lv2 (1609)`. This needs correction.

**Mechanics:**
- Enables HP/SP recovery while sitting, even when over 50% weight limit
- Recovery occurs every 10 seconds normally, every 20 seconds when 50-89% overweight
- Does NOT work at 90%+ weight
- Continues working during Fury status (when normal SP regen is disabled)
- This is why Asura Strike monks can recover SP via sitting even after the 5-min lockout

**Recovery Formula Per Level:**

| Level | HP Recovery | SP Recovery |
|-------|------------|------------|
| 1 | (MaxHP / 500) + 4 | (MaxSP / 500) + 2 |
| 2 | (MaxHP / 250) + 8 | (MaxSP / 250) + 4 |
| 3 | (MaxHP / 166) + 12 | (MaxSP / 166) + 6 |
| 4 | (MaxHP / 125) + 16 | (MaxSP / 125) + 8 |
| 5 | (MaxHP / 100) + 20 | (MaxSP / 100) + 10 |

**General Formula:**
```
HP = (MaxHP / (500 / level)) + (4 * level) = (MaxHP * level / 500) + (4 * level)
SP = (MaxSP / (500 / level)) + (2 * level) = (MaxSP * level / 500) + (2 * level)
```

**Implementation Notes:**
- Requires a "sitting" state system (player.isSitting flag)
- Interacts with the weight system (`/sabrimmo-weight`) -- works at 50-89% but not 90%+
- During Fury status, normal SP regen is disabled, but Spiritual Cadence bypass works
- During Asura SP lockout (5 min), Spiritual Cadence also bypasses the lockout

**Current Definition Corrections Needed:**
- `prerequisites: [{ skillId: 1601, level: 2 }]` should be `[{ skillId: 1609, level: 2 }]` (Blade Stop Lv2)

---

### 8. ABSORB SPIRIT SPHERE (ID 1607) -- Active, Single/Self Target

| Property | Value |
|----------|-------|
| rAthena ID | 262 (MO_ABSORBSPIRITS) |
| Type | Active, Supportive |
| Max Level | 1 |
| Prerequisites | Summon Spirit Sphere Lv5 (1601) |
| Target | Self or Enemy (player) |
| Range | 9 cells (450 UE units) |
| SP Cost | 5 |
| Cast Time | 2000ms |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Tree Position | Row 0, Col 3 |

**Mechanics:**

**On Self:**
- Consumes all of own spirit spheres
- Recovers 7 SP per sphere absorbed

**On Other Players (PvP):**
- Absorbs all of target player's spirit spheres
- Caster recovers 7 SP per sphere absorbed
- Target loses all spheres
- 100% success rate

**On Monsters:**
- 10% chance to drain SP equal to 2x the monster's level
- If successful: `SP gained = monster.level * 2`
- Does not actually drain spheres (monsters don't have spheres)

**Server Handler:**
```js
if (skill.name === 'absorb_spirit_sphere') {
    // Self-cast: absorb own spheres
    if (!targetId || targetId === characterId) {
        const spGained = player.spiritSpheres * 7;
        player.mana = Math.min(player.maxMana, player.mana + spGained);
        player.spiritSpheres = 0;
        // Broadcast sphere:update, health_update
    }
    // On enemy monster: 10% chance
    else if (isEnemy) {
        const enemy = enemies.get(targetId);
        if (Math.random() < 0.10) {
            const spGained = (enemy.level || 1) * 2;
            player.mana = Math.min(player.maxMana, player.mana + spGained);
        }
    }
    // On other player (PvP): steal all spheres
    else {
        // Future PvP implementation
    }
}
```

**Current Definition Corrections Needed:**
- `castTime: 0` should be `castTime: 2000` (2 second cast time)
- `cooldown: 2000` should be `cooldown: 0` (no cooldown). Note: the 2s cast time IS the effective rate limiter
- `effectValue: 10` -- represents SP per sphere (but actual mechanic gives 7 SP from self, not 10). Should be 7.

---

### 9. DODGE (ID 1608) -- Passive

| Property | Value |
|----------|-------|
| rAthena ID | 265 (MO_DODGE) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Iron Fists Lv5 (1600), Summon Spirit Sphere Lv5 (1601) |
| Tree Position | Row 1, Col 2 |

**FLEE Bonus Per Level (canonical values):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| FLEE | +1 | +3 | +4 | +6 | +7 | +9 | +10 | +12 | +13 | +15 |

**Formula:** `floor(1.5 * level)`

**Implementation:**
- Added to `getPassiveSkillBonuses()` as `bonusFLEE`
- Identical pattern to Thief's Improve Dodge (ID 501) which uses `(i+1)*3`
- But Monk Dodge uses `floor(1.5 * level)` -- different formula!

**Current Definition Corrections Needed:**
- `effectValue: 1+i` gives 1,2,3,4,5,6,7,8,9,10 -- WRONG
- Should be `effectValue: Math.floor(1.5 * (i+1))` giving 1,3,4,6,7,9,10,12,13,15
- Since `genLevels` may not support `Math.floor()`, use manual array:
  ```js
  levels: [1,3,4,6,7,9,10,12,13,15].map((v,i) => ({
      level: i+1, spCost: 0, castTime: 0, cooldown: 0, effectValue: v, duration: 0
  }))
  ```

---

### 10. BLADE STOP (ID 1609) -- Active, Self

| Property | Value |
|----------|-------|
| rAthena ID | 269 (MO_BLADESTOP) |
| Type | Active, Self (stance/counter) |
| Max Level | 5 |
| Prerequisites | Dodge Lv5 (1608) |
| SP Cost | 10 (all levels) |
| Sphere Cost | 1 per activation (consumed whether it catches an attack or not) |
| Cast Time | 0 |
| Tree Position | Row 2, Col 2 |

**Per-Level Data:**

| Level | Catch Window | Lock Duration | Skills Usable During Lock |
|-------|-------------|--------------|--------------------------|
| 1 | 0.5s (500ms) | 20s | None |
| 2 | 0.7s (700ms) | 30s | Finger Offensive |
| 3 | 0.9s (900ms) | 40s | + Investigate |
| 4 | 1.1s (1100ms) | 50s | + Chain Combo |
| 5 | 1.3s (1300ms) | 60s | + Asura Strike |

**Catch Window Formula:** `300 + 200 * level` ms
**Lock Duration Formula:** `10000 + 10000 * level` ms

**Mechanics:**
1. Player activates Blade Stop -- enters "catching" stance for the catch window duration
2. If an enemy melee auto-attack hits during the catch window, the attack is caught
3. Both the Monk and the attacker are locked in place (cannot move, cannot auto-attack)
4. During the lock, ONLY the Monk can attack using skills determined by Blade Stop level
5. The attacker cannot act at all during the lock
6. Cannot catch skills (only auto-attacks)
7. Cannot catch ranged attacks (melee only -- caster must be in melee range of attacker)
8. Sphere is consumed on activation regardless of whether an attack is caught
9. If no attack is caught during the window, the skill ends with no effect (sphere still consumed)

**Root Status (Lock) Server Logic:**
```js
// When blade_stop catches an attack:
player.rootLock = {
    active: true,
    targetId: attackerId,           // The caught attacker
    expiresAt: Date.now() + lockDuration,
    bladeStopLevel: learnedLevel    // Determines which skills are usable
};
// Caught attacker:
attacker.rootedBy = characterId;
attacker.rootedUntil = Date.now() + lockDuration;
// Both cannot move or auto-attack during this time
```

**Current Definition Status:** Mostly correct.
- `spCost: 10` -- CORRECT
- `duration: 20000+i*10000` gives 20000,30000,40000,50000,60000 -- CORRECT

---

### 11. CHAIN COMBO (ID 1610) -- Active, Combo Only

| Property | Value |
|----------|-------|
| rAthena ID | 272 (MO_CHAINCOMBO) |
| Type | Active, Offensive, Physical (Combo skill) |
| Max Level | 5 |
| Prerequisites | Triple Attack Lv5 (1603) |
| Target | Single Enemy (inherited from combo chain) |
| Range | 2 cells (melee) |
| Cast Time | 0 (instant -- combo skill) |
| Number of Hits | 4 |
| Element | Weapon element |
| isCombo | true |
| Tree Position | Row 2, Col 1 |

**Per-Level Data:**

| Level | Total ATK% | Per-Hit ATK% | SP Cost |
|-------|-----------|-------------|---------|
| 1 | 200% | 50% | 11 |
| 2 | 250% | 62.5% | 12 |
| 3 | 300% | 75% | 13 |
| 4 | 350% | 87.5% | 14 |
| 5 | 400% | 100% | 15 |

**ATK% Formula:** `100 + 50 * level` (matches Finger Offensive scaling -- but Chain Combo has 4 hits)
Hmm, wait. Let me re-verify. RateMyServer shows 200%, 250%, 300%, 350%, 400%. But the formula field shows different. Let me use the wiki values: 200-400%.

**SP Cost Formula:** `10 + level`

**Activation Rules:**
- Can ONLY be activated during the combo window after Triple Attack
- Can also be activated during Blade Stop Lv4+ lock
- Inherited target from the combo chain (auto-attack target)
- Cannot be freely targeted -- always hits the combo chain target

**Combo Delay (window to chain into Combo Finish):**
```
comboDelay = 1.3 - (AGI * 0.004) - (DEX * 0.002)   // seconds
```

**Current Definition Status:**
- `effectValue: 200+i*50` gives 200,250,300,350,400 -- CORRECT
- `spCost: 11+i` gives 11,12,13,14,15 -- CORRECT
- `isCombo: true` -- CORRECT
- `castTime: 0` -- CORRECT

---

### 12. CRITICAL EXPLOSION (ID 1611) -- Active, Self Buff (Fury)

| Property | Value |
|----------|-------|
| rAthena ID | 270 (MO_EXPLOSIONSPIRITS) |
| Type | Active, Self Buff |
| Max Level | 5 |
| Prerequisites | Absorb Spirit Sphere Lv1 (1607) |
| SP Cost | 15 (all levels) |
| Cast Time | 0 |
| Duration | 180 seconds (180,000 ms, all levels) |
| Sphere Cost | 5 (all consumed on activation) |
| Tree Position | Row 1, Col 3 |

**IMPORTANT:** Current definition lists prerequisites as `[{ skillId: 1601, level: 5 }, { skillId: 1600, level: 5 }]` (Summon Spirit Sphere Lv5 + Iron Fists Lv5). Canonical prerequisite is `Absorb Spirit Sphere Lv1 (1607)`. This needs correction.

**CRIT Bonus Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| CRIT | +10 | +12.5 | +15 | +17.5 | +20 |

**CRIT Formula:** `7.5 + 2.5 * level`

Since RO CRIT is displayed as integers but calculated with decimals:
- Implementation can use the decimal values directly in the `critBonus` buff modifier
- Or round: 10, 13, 15, 18, 20 (alternate interpretation)

**Fury Status Effects:**
1. +CRIT bonus per skill level
2. Enables use of Asura Strike
3. **Disables natural SP regeneration** while active
4. Spiritual Cadence (sitting regen) still works during Fury
5. Snap (Body Relocation) costs no spheres during Fury
6. Visual aura appears around character

**Buff Application:**
```js
applyBuff(player, {
    skillId: 1611,
    name: 'critical_explosion',  // aka 'fury'
    casterId: characterId,
    critBonus: 7.5 + 2.5 * learnedLevel,
    disableSPRegen: true,
    enableAsura: true,
    freeSnap: true,              // Snap costs no spheres during Fury
    duration: 180000
});
```

**Current Definition Corrections Needed:**
- `prerequisites` should be `[{ skillId: 1607, level: 1 }]` (Absorb Spirit Sphere Lv1)
- `effectValue: 10+i*2` gives 10,12,14,16,18 -- should be `effectValue: 7.5+2.5*(i+1)` giving 10, 12.5, 15, 17.5, 20. Since effectValue is typically integer, can store as `[10, 13, 15, 18, 20]` (rounded) or handle the decimal in the handler.

---

### 13. STEEL BODY (ID 1612) -- Active, Self Buff

| Property | Value |
|----------|-------|
| rAthena ID | 268 (MO_STEELBODY) |
| Type | Active, Self Buff |
| Max Level | 5 |
| Prerequisites | Combo Finish Lv3 (1613) |
| SP Cost | 200 (all levels) |
| Cast Time | 5000ms (uninterruptible) |
| Sphere Cost | 5 (all consumed on activation) |
| Tree Position | Row 3, Col 2 |

**IMPORTANT:** Current definition lists prerequisites as `[{ skillId: 1611, level: 3 }, { skillId: 1609, level: 3 }]` (Critical Explosion Lv3 + Blade Stop Lv3). Canonical prerequisite is `Combo Finish Lv3 (1613)`. This needs correction.

**Duration Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration | 30s | 60s | 90s | 120s | 150s |

**Duration Formula:** `30 * level` seconds

**Effects When Active:**
1. Sets hard DEF to 90 (capped, equipment DEF is overridden)
2. Sets hard MDEF to 90 (capped, equipment MDEF is overridden)
3. Reduces ASPD by 25%
4. Reduces movement speed by 25%
5. **Cannot use active skills** (all active skills are disabled)
6. Passive skills continue to function normally (Triple Attack, Dodge, Iron Fists)
7. Items ARE usable (potions, etc.)
8. Stacks with Assumptio (Priest buff)

**Buff Application:**
```js
applyBuff(player, {
    skillId: 1612,
    name: 'steel_body',
    casterId: characterId,
    overrideHardDEF: 90,
    overrideHardMDEF: 90,
    aspdReduction: 0.25,        // -25% ASPD
    moveSpeedReduction: 25,     // -25% move speed
    blockActiveSkills: true,    // Cannot use active skills
    duration: learnedLevel * 30000
});
```

**Current Definition Corrections Needed:**
- `prerequisites` should be `[{ skillId: 1613, level: 3 }]` (Combo Finish Lv3)
- `duration: 30000+i*30000` gives 30000,60000,90000,120000,150000 -- CORRECT

---

### 14. COMBO FINISH (ID 1613) -- Active, Combo Only

| Property | Value |
|----------|-------|
| rAthena ID | 273 (MO_COMBOFINISH) |
| Type | Active, Offensive, Physical (Combo skill) |
| Max Level | 5 |
| Prerequisites | Chain Combo Lv3 (1610) |
| Target | Single Enemy (inherited from combo chain) + 5x5 AoE splash |
| Range | 2 cells (melee) |
| Cast Time | 0 (instant -- combo skill) |
| Number of Hits | 1 |
| Sphere Cost | 1 per use |
| Element | Weapon element |
| AoE | 5x5 cells (~250 UE units radius) splash around primary target |
| isCombo | true |
| Tree Position | Row 3, Col 1 |

**Per-Level Data:**

| Level | ATK% | SP Cost |
|-------|------|---------|
| 1 | 300% | 11 |
| 2 | 360% | 12 |
| 3 | 420% | 13 |
| 4 | 480% | 14 |
| 5 | 540% | 15 |

**ATK% Formula:** `240 + 60 * level`
**SP Cost Formula:** `10 + level`

**Activation Rules:**
- Can ONLY be activated during the combo window after Chain Combo
- Inherited target from combo chain
- Consumes 1 spirit sphere
- Has 5x5 AoE splash -- hits enemies around the primary target for the same damage
- The primary target and splash targets all take the listed ATK%
- Knockback of 5 cells on splash targets (NOT on primary target)

**Combo Window to Asura Strike:**
If Fury is active and at least 1 sphere remains after Combo Finish (need 5 before starting combo: Triple Attack costs 0, Chain Combo costs 0 spheres, Combo Finish costs 1 = 4 remaining), the combo window opens for Asura Strike.

**Current Definition Status:**
- `effectValue: 300+i*60` gives 300,360,420,480,540 -- CORRECT
- `spCost: 11+i` gives 11,12,13,14,15 -- CORRECT
- `isCombo: true` -- CORRECT

---

### 15. KI TRANSLATION (ID 1614) -- Quest Skill, Active

| Property | Value |
|----------|-------|
| rAthena ID | 1015 (MO_KITRANSLATION) |
| Type | Active, Supportive (Quest Skill) |
| Max Level | 1 |
| Target | Party Member |
| Range | 9 cells (450 UE units) |
| SP Cost | 40 |
| Cast Time | 2000ms |
| After-Cast Delay | 1000ms |
| Sphere Cost | 1 (transferred to target) |
| Tree Position | Row 4, Col 0 |

**Mechanics:**
- Transfers 1 spirit sphere from the Monk to a party member
- Target cannot exceed 5 spheres maximum
- Fails if caster has no spheres
- Gunslingers cannot receive spheres
- Requires party system (deferred)

**Current Definition Status:** CORRECT -- `spCost: 40, castTime: 2000, questSkill: true`

---

### 16. KI EXPLOSION (ID 1615) -- Quest Skill, Active

| Property | Value |
|----------|-------|
| rAthena ID | 1016 (MO_KIEXPLOSION) |
| Type | Active, Offensive, Physical (Quest Skill) |
| Max Level | 1 |
| Target | Single Enemy |
| Range | 1 cell (melee) |
| AoE | 3x3 cells around target |
| SP Cost | 20 |
| HP Cost | 10 |
| Cast Time | 0 |
| After-Cast Delay | 2000ms |
| Knockback | 5 cells (adjacent enemies only, NOT the primary target) |
| Stun | 70% chance, 2 second duration (adjacent enemies) |
| ATK% | 300% |
| Tree Position | Row 4, Col 1 |

**Mechanics:**
- Primary target takes 300% ATK damage
- Enemies adjacent to primary target (3x3 AoE) are knocked back 5 cells
- Adjacent enemies have 70% chance to be stunned for 2 seconds
- Primary target is NOT knocked back
- Costs 10 HP in addition to 20 SP
- Uses weapon element

**Current Definition Status:** Mostly correct. `effectValue: 300` is correct.

---

## Skill Tree and Prerequisites

### Canonical Skill Tree (Corrected)

```
Row 0: [Iron Fists] ---- [Summon Spirit Sphere] ---- [Spirits Recovery*] ---- [Absorb Spirit Sphere]
           |                      |                                                    |
           |                      |                                                    |
Row 1: [Investigate] --------- [Triple Attack*] ----------- [Dodge] ---------- [Critical Explosion]
           |                      |                            |
           |                      |                            |
Row 2: [Finger Offensive] -- [Chain Combo] ------------ [Blade Stop]
           |                      |                            |
           |                      |                            |
Row 3: [Asura Strike] ------ [Combo Finish] ----------- [Steel Body]

Row 4: [Ki Translation] ---- [Ki Explosion]
       (Quest Skills)
```

### Full Prerequisite Chain (Corrected)

| Skill | ID | Prerequisites |
|-------|----|--------------|
| Iron Fists | 1600 | Divine Protection Lv10 (401), Demon Bane Lv10 (413) |
| Summon Spirit Sphere | 1601 | Iron Fists Lv2 (1600) |
| Investigate | 1602 | Summon Spirit Sphere Lv5 (1601) |
| Triple Attack | 1603 | **Dodge Lv5 (1608)** |
| Finger Offensive | 1604 | Investigate Lv3 (1602) |
| Asura Strike | 1605 | Finger Offensive Lv3 (1604), Critical Explosion Lv3 (1611) |
| Spirits Recovery | 1606 | **Blade Stop Lv2 (1609)** |
| Absorb Spirit Sphere | 1607 | Summon Spirit Sphere Lv5 (1601) |
| Dodge | 1608 | Iron Fists Lv5 (1600), Summon Spirit Sphere Lv5 (1601) |
| Blade Stop | 1609 | Dodge Lv5 (1608) |
| Chain Combo | 1610 | Triple Attack Lv5 (1603) |
| Critical Explosion | 1611 | **Absorb Spirit Sphere Lv1 (1607)** |
| Steel Body | 1612 | **Combo Finish Lv3 (1613)** |
| Combo Finish | 1613 | Chain Combo Lv3 (1610) |
| Ki Translation | 1614 | Quest skill (no tree prerequisite) |
| Ki Explosion | 1615 | Quest skill (no tree prerequisite) |
| Body Relocation (Snap) | -- | NOT a Monk skill -- Champion only (2-2 class) |

**NOTE:** Body Relocation (Snap) is listed on some sources as a Monk skill, but it requires Asura Strike Lv3 + Spirits Recovery Lv2 + Steel Body Lv3 as prerequisites. In many implementations, it is Monk-accessible. For our implementation, it can be added as a Monk skill in a future phase or deferred to Champion class. Prerequisites: Asura Strike Lv3 (1605), Spirits Recovery Lv2 (1606), Steel Body Lv3 (1612).

### Prerequisite Corrections Summary

| Skill | Current Prerequisites | Correct Prerequisites |
|-------|----------------------|----------------------|
| Triple Attack (1603) | Iron Fists Lv5 (1600) | **Dodge Lv5 (1608)** |
| Spirits Recovery (1606) | Summon Spirit Sphere Lv2 (1601) | **Blade Stop Lv2 (1609)** |
| Critical Explosion (1611) | SSS Lv5 (1601) + IF Lv5 (1600) | **Absorb Spirit Sphere Lv1 (1607)** |
| Steel Body (1612) | CE Lv3 (1611) + BS Lv3 (1609) | **Combo Finish Lv3 (1613)** |

---

## New Systems Required

### System 1: Spirit Sphere State (REQUIRED -- HIGH Priority)

**Scope:** New player property and lifecycle management.

**Server Changes:**
- Add `spiritSpheres` (int, 0-5) and `sphereExpireTime` (timestamp) to player object on join
- Add sphere count to `buildFullStatsPayload()` so client knows about spheres
- Add sphere expiry check in the 1-second buff tick loop
- All sphere-consuming skills must validate `player.spiritSpheres >= cost` before executing
- Broadcast `sphere:update` when count changes

**Client Changes:**
- New VFX: orbiting holy sphere particles (1-5 orbs)
- Add sphere count display somewhere in HUD (or just VFX)
- Parse `sphere:update` events in a subsystem

**Effort:** Medium (server: small, client VFX: medium)

### System 2: Combo State Machine (REQUIRED -- HIGH Priority)

**Scope:** New combo tracking system in auto-attack tick and skill:use handler.

**Server Changes:**
- Add `comboState` object to player (see Combo System Architecture section above)
- In auto-attack tick: check for Triple Attack proc, open combo window on proc
- In `skill:use` handler: add combo validation path for Chain Combo (1610), Combo Finish (1613), and Asura Strike (1605) when `isCombo` is true
- Combo skills bypass cast time check (instant), bypass targeting (use combo target), bypass range check (already in melee)
- Combo window expiry check in combat tick (close window after timeout)
- New Socket.io event: `skill:combo_window` for client hotbar highlighting

**Client Changes:**
- Listen for `skill:combo_window` event
- Flash/pulse the next combo skill on hotbar during the window
- Auto-target: combo skills use the combo chain target, not a new click target

**Effort:** Large (core new system, touches auto-attack tick and skill handler)

### System 3: Custom Investigate Damage Pipeline (REQUIRED -- MEDIUM Priority)

**Scope:** New damage calculation function for Investigate (Occult Impaction).

**Server Changes:**
- New function `calculateInvestigateDamage()` that:
  1. Calculates StatusATK + WeaponATK normally
  2. Applies skill multiplier: `ATK * (1 + 0.75 * level)`
  3. Multiplies by target DEF: `* (hardDEF + softDEF) / 50`
  4. Applies card bonuses
  5. Applies size penalty
  6. Does NOT subtract DEF (the whole point)
  7. Always hits (skip HIT/FLEE)

**Effort:** Small (isolated function, clear formula)

### System 4: Asura Strike Custom Damage (REQUIRED -- MEDIUM Priority)

**Scope:** Custom damage formula for Asura Strike.

**Server Changes:**
- New function or inline logic:
  1. Calculate `(WeaponATK + StatusATK) * (8 + SP/10) + flatBonus`
  2. Apply card bonuses (race/element/size -- multiplicative)
  3. Apply element modifier (Neutral vs target)
  4. Do NOT apply DEF reduction
  5. Consume ALL SP, ALL spheres, remove Fury
  6. Apply 5-minute SP regen lockout
- Handle combo bypass (no cast time when used after Combo Finish)
- Handle Blade Stop bypass (no cast time during Root Lv5)

**Effort:** Medium (complex but well-defined formula)

### System 5: Blade Stop Lock System (DEFERRED -- LOW Priority)

**Scope:** Counter-attack stance that locks both combatants.

**Server Changes:**
- Blade Stop activation: set "catching" state for X milliseconds
- Hook into auto-attack processing: if target has blade_stop catching active, trigger root lock
- Root lock: immobilize both Monk and attacker, allow Monk to use skills per Blade Stop level
- Root lock expiry: release both
- Interact with movement: locked players cannot move

**Effort:** Large (complex interaction with auto-attack tick, movement system, skill system)

**Recommendation:** Defer to Phase 2 of Monk implementation. The combo system and basic skills work without Blade Stop.

### System 6: Steel Body Buff Modifiers (REQUIRED -- MEDIUM Priority)

**Scope:** Buff that overrides DEF/MDEF and restricts skill usage.

**Server Changes:**
- New buff type with `overrideHardDEF`, `overrideHardMDEF` fields
- In damage calculation: if target has `steel_body` buff, use 90 for hard DEF/MDEF instead of equipment values
- In skill:use handler: if player has `steel_body` buff, block all active skills
- In ASPD calculation: if `aspdReduction` buff modifier exists, apply it
- In movement: if `moveSpeedReduction` buff modifier exists, reduce speed

**Effort:** Medium (buff modifiers need plumbing through multiple systems)

### System 7: Fury Buff with SP Regen Disable (REQUIRED -- MEDIUM Priority)

**Scope:** Fury status that enables Asura and disables SP regen.

**Server Changes:**
- New buff with `disableSPRegen: true` flag
- In HP/SP regen tick: check for `disableSPRegen` flag, skip SP regen if set
- Exception: Spirits Recovery (passive sitting regen) bypasses this flag
- When Asura Strike is used, remove Fury buff and apply `asuraRegenLockout`
- `asuraRegenLockout` (5 min): blocks ALL SP regen including items... actually, items still work. Only natural regen is blocked.

**Effort:** Small (flag check in regen tick)

### System 8: Spirits Recovery Sitting Regen (DEFERRED -- LOW Priority)

**Scope:** Passive regen while sitting that bypasses weight and Fury restrictions.

**Server Changes:**
- Requires "sitting" state system (`player.isSitting` flag)
- When sitting and player has Spirits Recovery, apply bonus HP/SP regen
- Bypasses weight threshold restrictions (works 0-89% weight, not 90%+)
- Bypasses Fury SP regen disable
- Bypasses Asura SP regen lockout

**Effort:** Medium (needs sitting system)

**Recommendation:** Defer until sitting system is implemented.

---

## Existing Skill Definition Corrections

The following corrections need to be made to the Monk skill definitions in `server/src/ro_skill_data_2nd.js`:

### Investigate (ID 1602)
```
CHANGE: castTime from 500 to 1000
CHANGE: cooldown: 500 -> afterCastDelay: 500, cooldown: 0
CHANGE: effectValue from 150+i*50 to manual [175, 250, 325, 400, 475]
CHANGE: spCost from 10+i*3 to manual [10, 14, 17, 19, 20]
```

### Triple Attack (ID 1603)
```
CHANGE: prerequisites from [{ skillId: 1600, level: 5 }] to [{ skillId: 1608, level: 5 }]
ADD: new field tripleAttackChance per level (store as extra field or compute from effectValue)
```
Note: effectValue 120-300 is the ATK% and is CORRECT. The proc chance (29-20%) must be tracked separately.

### Finger Offensive (ID 1604)
```
CHANGE: spCost from 10+i*3 to flat 10
CHANGE: cooldown: 1000 -> afterCastDelay: 500, cooldown: 0
NOTE: castTime: 1000 is the BASE; actual = (1 + spheres) * 1000 calculated at runtime
```

### Asura Strike (ID 1605)
```
CHANGE: effectValue from 250+i*150 to manual [400, 550, 700, 850, 1000]
CHANGE: cooldown: 3000 to afterCastDelay per level [3000, 2500, 2000, 1500, 1000]
```

### Spirits Recovery (ID 1606)
```
CHANGE: prerequisites from [{ skillId: 1601, level: 2 }] to [{ skillId: 1609, level: 2 }]
```

### Absorb Spirit Sphere (ID 1607)
```
CHANGE: castTime from 0 to 2000
CHANGE: cooldown from 2000 to 0
CHANGE: effectValue from 10 to 7 (SP per sphere when absorbing own)
```

### Dodge (ID 1608)
```
CHANGE: effectValue from 1+i to manual [1,3,4,6,7,9,10,12,13,15]
```

### Critical Explosion (ID 1611)
```
CHANGE: prerequisites from [{ skillId: 1601, level: 5 }, { skillId: 1600, level: 5 }]
        to [{ skillId: 1607, level: 1 }]
CHANGE: effectValue from 10+i*2 to manual [10, 13, 15, 18, 20] (rounded from 10, 12.5, 15, 17.5, 20)
```

### Steel Body (ID 1612)
```
CHANGE: prerequisites from [{ skillId: 1611, level: 3 }, { skillId: 1609, level: 3 }]
        to [{ skillId: 1613, level: 3 }]
```

---

## Implementation Priority

### Phase 1: Core Foundation (REQUIRED -- implement first)

1. **Spirit Sphere System** -- player.spiritSpheres, sphere:update event, sphere expiry
2. **Summon Spirit Sphere handler** -- Add/track spheres, validate max
3. **Iron Fists passive** -- Add to `getPassiveSkillBonuses()` for knuckle/bare hand ATK
4. **Dodge passive** -- Add to `getPassiveSkillBonuses()` for FLEE bonus
5. **Critical Explosion (Fury) handler** -- Buff with CRIT bonus, SP regen disable, enable Asura
6. **Absorb Spirit Sphere handler** -- Self: 7 SP/sphere, Monster: 10% chance 2x level SP
7. **Skill definition corrections** -- Fix all prerequisites, effectValues, SP costs per above section

### Phase 2: Offensive Skills (REQUIRED -- core combat)

1. **Investigate handler** -- Custom damage pipeline (DEF multiplier, always hits, Neutral)
2. **Finger Offensive handler** -- Multi-hit ranged (1 hit per sphere, variable cast time)
3. **Asura Strike handler** -- Custom massive damage formula, consume all SP/spheres, regen lockout
4. **Ki Explosion handler** -- 300% AoE, knockback, stun

### Phase 3: Combo System (REQUIRED -- core gameplay identity)

1. **Triple Attack passive in auto-attack tick** -- Proc check, 3-hit damage, open combo window
2. **Combo state machine** -- Track combo chain, validate timing windows
3. **Chain Combo handler** -- 4-hit combo skill, validate combo state
4. **Combo Finish handler** -- AoE splash combo skill, 1 sphere cost
5. **Asura Strike combo bypass** -- No cast time when used in combo chain
6. **skill:combo_window event** -- Client notification for hotbar highlighting

### Phase 4: Buffs and Defensive (MEDIUM priority)

1. **Steel Body handler** -- DEF/MDEF override, ASPD/speed reduction, skill block
2. **Steel Body buff modifiers** -- Plumb through damage calc and skill validation
3. **Asura regen lockout** -- 5-minute natural SP regen block
4. **Fury SP regen disable** -- Block SP regen during Fury state

### Phase 5: Advanced Systems (DEFERRED)

1. **Blade Stop** -- Counter stance, root lock, skill-during-lock
2. **Spirits Recovery** -- Sitting regen bypass
3. **Ki Translation** -- Sphere transfer (needs party system)
4. **Body Relocation (Snap)** -- Instant teleport (may be Champion-only)
5. **Client sphere VFX** -- Orbiting holy particles

---

## Integration Points

### Existing Systems Affected

| System | Integration Point |
|--------|------------------|
| `getPassiveSkillBonuses()` | Add Iron Fists (+3 ATK/lv with knuckle/bare hand), Dodge (FLEE), Triple Attack proc chance |
| `getEffectiveStats()` | Add spirit sphere ATK bonus (+3 per sphere) |
| Auto-attack tick (combat tick loop) | Triple Attack proc check, combo window tracking |
| `skill:use` handler | 11 new active skill handlers, combo validation |
| `getBuffStatModifiers()` | Fury (CRIT, SP regen), Steel Body (DEF/MDEF override, ASPD, skill block) |
| HP/SP regen tick | Fury SP regen disable, Asura regen lockout, Spirits Recovery bypass |
| `buildFullStatsPayload()` | Include `spiritSpheres`, `maxSphritSpheres` |
| Damage pipeline | Investigate custom calc, Asura custom calc |
| `ro_damage_formulas.js` | New export: `calculateInvestigateDamage()` |
| SkillVFXData.cpp | 11 new VFX configs for active Monk skills |
| CombatActionSubsystem | Parse new combo events |
| HotbarSubsystem | Combo window hotbar highlighting |

### Socket.io Events (New)

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `sphere:update` | Server -> Zone | `{ characterId, sphereCount, maxSpheres }` | Sphere count changed |
| `skill:combo_window` | Server -> Caster | `{ nextSkillId, windowMs }` | Combo window opened -- highlight hotbar |
| `skill:combo_expired` | Server -> Caster | `{}` | Combo window closed |

### Buff System Additions

| Buff Name | Effects | Duration |
|-----------|---------|----------|
| `critical_explosion` | +CRIT, disableSPRegen, enableAsura, freeSnap | 180s |
| `steel_body` | overrideHardDEF=90, overrideHardMDEF=90, aspdReduction=25%, moveSpeedReduction=25%, blockActiveSkills | 30-150s |
| `asura_regen_lockout` | disableSPRegen, disableSPItems=false | 300s (5 min) |
| `blade_stop_catching` | Stance -- waiting for melee attack | 0.5-1.3s |
| `root_lock` | Immobilize both combatants | 20-60s |

---

## Monk-Specific Constants

```js
// Spirit Sphere constants
const SPHERE_ATK_BONUS = 3;              // +3 ATK per sphere
const SPHERE_MAX_MONK = 5;               // Max spheres for Monk
const SPHERE_MAX_CHAMPION = 10;          // Max for Champion (future)
const SPHERE_DURATION = 600000;           // 10 minutes in ms

// Combo system constants
const COMBO_BASE_DELAY = 1.3;            // Base combo window in seconds
const COMBO_AGI_FACTOR = 0.004;          // AGI reduction per point
const COMBO_DEX_FACTOR = 0.002;          // DEX reduction per point

// Asura Strike constants
const ASURA_SP_REGEN_LOCKOUT = 300000;   // 5 minutes in ms
const ASURA_ATK_MULTIPLIER_BASE = 8;     // Base ATK multiplier
const ASURA_SP_DIVISOR = 10;             // SP is divided by 10 and added to multiplier
const ASURA_FLAT_BASE = 250;             // Base flat bonus
const ASURA_FLAT_PER_LEVEL = 150;        // Flat bonus per skill level

// Investigate constants
const INVESTIGATE_DEF_DIVISOR = 50;      // DEF+VIT divided by 50

// Fury CRIT formula
const FURY_CRIT_BASE = 7.5;
const FURY_CRIT_PER_LEVEL = 2.5;

// Blade Stop catch window
const BLADE_STOP_CATCH_BASE = 300;       // Base catch window in ms
const BLADE_STOP_CATCH_PER_LEVEL = 200;  // Additional ms per level

// Blade Stop lock duration
const BLADE_STOP_LOCK_BASE = 10000;      // Base lock duration in ms
const BLADE_STOP_LOCK_PER_LEVEL = 10000; // Additional ms per level

// Combo skill valid chains
const COMBO_CHAINS = {
    1603: [1610],        // Triple Attack -> Chain Combo
    1610: [1613],        // Chain Combo -> Combo Finish
    1613: [1605]         // Combo Finish -> Asura Strike (requires Fury + spheres)
};

// Blade Stop skills available per level
const BLADE_STOP_SKILLS = {
    1: [],               // No skills at Lv1
    2: [1604],           // Finger Offensive
    3: [1604, 1602],     // + Investigate
    4: [1604, 1602, 1610], // + Chain Combo
    5: [1604, 1602, 1610, 1605] // + Asura Strike
};

// Investigate SP costs (irregular, must be manual)
const INVESTIGATE_SP_COSTS = [10, 14, 17, 19, 20];

// Asura Strike after-cast delays per level
const ASURA_ACD = [3000, 2500, 2000, 1500, 1000];

// Asura Strike flat bonuses per level
const ASURA_FLAT_BONUS = [400, 550, 700, 850, 1000];

// Dodge FLEE bonuses per level (floor(1.5 * level))
const DODGE_FLEE = [1, 3, 4, 6, 7, 9, 10, 12, 13, 15];

// Critical Explosion CRIT per level (7.5 + 2.5*level, rounded)
const FURY_CRIT = [10, 13, 15, 18, 20];

// Triple Attack proc chance per level (30 - level)
const TRIPLE_ATTACK_CHANCE = [29, 28, 27, 26, 25, 24, 23, 22, 21, 20];
```

---

## Sources

- [Monk - iRO Wiki Classic](https://irowiki.org/classic/Monk)
- [Monk Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=15)
- [Guillotine Fist - iRO Wiki Classic](https://irowiki.org/classic/Guillotine_Fist)
- [Guillotine Fist - iRO Wiki](https://irowiki.org/wiki/Guillotine_Fist)
- [Occult Impaction - iRO Wiki Classic](https://irowiki.org/classic/Occult_Impaction)
- [Occult Impaction - iRO Wiki](https://irowiki.org/wiki/Occult_Impaction)
- [Occult Impaction damage - rAthena Issue #1641](https://github.com/rathena/rathena/issues/1641)
- [Raging Trifecta Blow - iRO Wiki Classic](https://irowiki.org/classic/Raging_Trifecta_Blow)
- [Raging Quadruple Blow - iRO Wiki Classic](https://irowiki.org/classic/Raging_Quadruple_Blow)
- [Raging Thrust - iRO Wiki Classic](https://irowiki.org/classic/Raging_Thrust)
- [Throw Spirit Sphere - iRO Wiki Classic](https://irowiki.org/classic/Throw_Spirit_Sphere)
- [Fury - iRO Wiki Classic](https://irowiki.org/classic/Fury)
- [Fury - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=270)
- [Mental Strength - iRO Wiki Classic](https://irowiki.org/classic/Mental_Strength)
- [Mental Strength - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=268)
- [Root - iRO Wiki Classic](https://irowiki.org/classic/Root)
- [Snap - iRO Wiki Classic](https://irowiki.org/classic/Snap)
- [Summon Spirit Sphere - iRO Wiki Classic](https://irowiki.org/classic/Summon_Spirit_Sphere)
- [Spiritual Sphere Absorption - iRO Wiki Classic](https://irowiki.org/classic/Spiritual_Sphere_Absorption)
- [Spiritual Cadence - iRO Wiki Classic](https://irowiki.org/classic/Spiritual_Cadence)
- [Flee (Skill) - iRO Wiki Classic](https://irowiki.org/classic/Flee_(Skill))
- [Spiritual Bestowment - iRO Wiki Classic](https://irowiki.org/classic/Spiritual_Bestowment)
- [Excruciating Palm - iRO Wiki Classic](https://irowiki.org/classic/Excruciating_Palm)
- [Asura Strike - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=271)
- [Asura Strike - rAthena Pre-Renewal DB](https://pre.pservero.com/skill/MO_EXTREMITYFIST)
- [Throw Spirit Sphere - rAthena Pre-Renewal DB](https://pre.pservero.com/skill/MO_FINGEROFFENSIVE)
- [Investigate - rAthena Pre-Renewal DB](https://pre.pservero.com/skill/MO_INVESTIGATE)
- [Monk Combo Guide - GameFAQs (LordTopak)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/37726)
- [Monk Guide - GameFAQs (CalxZinra)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/36497)
- [rAthena battle.cpp source](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
