# Assassin & Assassin Cross — Complete Pre-Renewal Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Assassin_Skills_Audit](Assassin_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Sources:** iRO Wiki Classic, iRO Wiki (renewal for cross-ref), RateMyServer Pre-RE DB, rAthena pre-re source, RagnaCloneDocs/03_Skills_Complete.md

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [What Is Already Implemented](#what-is-already-implemented)
3. [Assassin Skills (2nd Class)](#assassin-skills-2nd-class)
   - [Katar Mastery](#1-katar-mastery-id-1100)
   - [Sonic Blow](#2-sonic-blow-id-1101)
   - [Grimtooth](#3-grimtooth-id-1102)
   - [Cloaking](#4-cloaking-id-1103)
   - [Poison React](#5-poison-react-id-1104)
   - [Venom Dust](#6-venom-dust-id-1105)
   - [Sonic Acceleration](#7-sonic-acceleration-id-1106)
   - [Righthand Mastery](#8-righthand-mastery-id-1107)
   - [Lefthand Mastery](#9-lefthand-mastery-id-1108)
   - [Enchant Poison](#10-enchant-poison-id-1109)
   - [Venom Splasher](#11-venom-splasher-id-1110)
   - [Throw Venom Knife](#12-throw-venom-knife-id-1111)
4. [Assassin Cross Skills (Transcendent)](#assassin-cross-skills-transcendent)
   - [Advanced Katar Mastery](#13-advanced-katar-mastery)
   - [Create Deadly Poison](#14-create-deadly-poison)
   - [Enchant Deadly Poison](#15-enchant-deadly-poison-edp)
   - [Soul Destroyer](#16-soul-destroyer-soul-breaker)
   - [Meteor Assault](#17-meteor-assault)
5. [Skill Tree & Prerequisites](#skill-tree--prerequisites)
6. [Existing Skill Definition Audit](#existing-skill-definition-audit)
7. [New Systems Required](#new-systems-required)
8. [Existing Systems That Can Be Reused](#existing-systems-that-can-be-reused)
9. [Implementation Priority](#implementation-priority)
10. [Assassin-Specific Constants & Data Tables](#assassin-specific-constants--data-tables)
11. [Integration Points](#integration-points)
12. [Sources](#sources)

---

## Class Overview

| Property | Value |
|----------|-------|
| Base Class | Thief |
| Transcendent | Assassin Cross |
| Primary Weapons | Katar (unique to Assassin/SinX), Dagger (dual wield) |
| Unique Mechanic | Dual Wielding (two 1H weapons simultaneously) |
| Play Styles | Katar crit build, Dual wield build, Sonic Blow burst, Soul Breaker hybrid (SinX) |
| Key Stats | STR (damage), AGI (ASPD/FLEE), DEX (HIT), LUK (crit with katar) |

**Identity:** The Assassin is a melee DPS class that excels at burst damage (Sonic Blow), stealth play (Cloaking/Hiding), and poison mechanics. It is the ONLY class that can dual wield weapons and the ONLY class that uses Katar weapons. Katars have a unique property: they double the critical rate from LUK.

**Katar Critical Bonus:** When wielding a Katar, the CRI from LUK is doubled. This is why crit Assassin builds stack LUK. Formula: `CRI = 1 + floor(LUK * 0.3) * 2` with Katar (the `* 2` is katar-specific). This is already handled in the damage formula via the existing katar detection system.

---

## What Is Already Implemented

### Dual Wield System -- COMPLETE
See `MEMORY.md` -> "Dual Wield System" section. Full implementation:
- Server helpers: `canDualWield()`, `isDualWielding()`, `isValidLeftHandWeapon()`, `isKatar()`
- Player fields: `equippedWeaponRight`, `equippedWeaponLeft`, `cardModsRight`, `cardModsLeft`
- ASPD: Dual wield `WD = floor((WD_R + WD_L) * 7/10)`, cap 190
- Combat tick: Both hands hit per cycle, mastery penalties applied
- Left hand: `forceHit` (always hits), `noCrit` (never crits), per-hand cards/element
- `combat:damage` extended: `damage2`, `isDualWield`, `isCritical2`, `element2`
- Client: `EquipSlots::WeaponLeft`, dynamic shield/Left Hand slot, dual ATK display

### Skill Definitions -- DEFINED (12 skills in `ro_skill_data_2nd.js`)
All 12 Assassin skills are defined in `server/src/ro_skill_data_2nd.js` (IDs 1100-1111):
- 3 passives: Katar Mastery (1100), Righthand Mastery (1107), Lefthand Mastery (1108)
- 2 quest skills: Sonic Acceleration (1106), Throw Venom Knife (1111)
- 7 active skills: Sonic Blow (1101), Grimtooth (1102), Cloaking (1103), Poison React (1104), Venom Dust (1105), Enchant Poison (1109), Venom Splasher (1110)

### Passive Handlers -- IMPLEMENTED (3 passives)
In `getPassiveSkillBonuses()` in `index.js`:
- Katar Mastery (1100): `+3 ATK/level` with Katars
- Righthand Mastery (1107): `rightHandMasteryLv` stored for combat tick
- Lefthand Mastery (1108): `leftHandMasteryLv` stored for combat tick

### Skill Handlers -- NONE IMPLEMENTED
No `skill:use` handlers exist for any Assassin active skills. All 9 active skills need handlers.

### Buff System -- Cloaking buff defined
`ro_buff_system.js` has `cloaking` buff entry with `stackRule: 'refresh'`, `category: 'buff'`. The `getCombinedModifiers()` function sets `isHidden: true` for both `hiding` and `cloaking` buffs.

### Hiding System -- Partially implemented (Thief)
Hiding toggle exists from Thief implementation but has significant gaps (see Thief Audit doc). Cloaking will share and extend the hiding mechanics.

---

## Assassin Skills (2nd Class)

### 1. KATAR MASTERY (ID 1100)

| Property | Value |
|----------|-------|
| rAthena ID | 134 |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |
| Weapon | Katar class only |

**Effect:** +3 flat ATK per level when wielding a Katar weapon. This bonus ignores DEF (added as mastery bonus, same as Sword Mastery).

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK Bonus | +3 | +6 | +9 | +12 | +15 | +18 | +21 | +24 | +27 | +30 |

**Status:** IMPLEMENTED. Handler in `getPassiveSkillBonuses()`: `bonusATK += kmLv * 3` when `wType === 'katar'`.

**Skill Definition Audit:** CORRECT. No changes needed.

---

### 2. SONIC BLOW (ID 1101)

| Property | Value |
|----------|-------|
| rAthena ID | 136 |
| Type | Active, Offensive |
| Target | Single Enemy |
| Range | Melee (1 cell = 50 UE units) |
| Element | Weapon element |
| Weapon Requirement | Katar class ONLY |
| Cast Time | 0 (instant) |
| After-Cast Delay | 2 seconds |
| Prerequisites | Katar Mastery Lv4 |
| Hit Count | 8 visual hits, calculated as ONE damage instance |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 440 | 480 | 520 | 560 | 600 | 640 | 680 | 720 | 760 | 800 |
| SP Cost | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 | 32 | 34 |
| Stun% | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |

**Formulas:**
- ATK%: `400 + 40 * SkillLv`
- SP Cost: `14 + 2 * SkillLv`
- Stun Chance: `10 + 2 * SkillLv` %

**Special Mechanics:**
1. **Katar required** -- skill fails if not wielding a Katar
2. **Can miss** -- subject to normal HIT/FLEE check (not forceHit)
3. **Can be elemental** -- uses weapon element (e.g., Enchant Poison makes it Poison)
4. **Stun on hit** -- applies Stun status with level-based chance. Boss protocol monsters immune.
5. **8 hits visual** -- damage is calculated as ONE hit but displayed as 8 rapid hits on client
6. **2-second animation** -- the animation itself takes 2 seconds and cannot be reduced. This is implemented as the after-cast delay.
7. **ASPD interaction** -- ASPD affects the delay BEFORE the animation starts (invisible "cast time"), not the animation speed itself. For our implementation, the 2s ACD covers this.

**EDP Interaction (Pre-Renewal):**
- EDP DOES affect Sonic Blow
- In pre-renewal, the skill modifier is HALVED when EDP is active: `effectiveATK% = ATK% / 2`
- So Sonic Blow Lv10 with EDP Lv5: `800% / 2 * 4.0 = 1600%` (still a net gain over 800%)

**Sonic Acceleration Interaction:**
- Sonic Acceleration adds +50% damage multiplicatively (pre-renewal value -- some servers updated to +90%)
- For our implementation: use the classic +50% value
- Also adds +50% to final HIT rate (multiplicative, not additive)
- With Sonic Acceleration: `SB_ATK% = baseSBATK * 1.5`, `effectiveHitRate = hitRate * 1.5`

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists.

**Skill Definition Audit:**
- `spCost: 16` is WRONG -- SP scales `14 + 2*Lv`. Lv1 should be 16, Lv2=18, etc. but genLevels uses `spCost: 16` (flat). NEEDS FIX to `spCost: 14 + (i+1)*2`
- `cooldown: 2000` -- this is listed as cooldown but should be `afterCastDelay: 2000, cooldown: 0`
- `effectValue: 440+i*40` -- CORRECT
- `range: 150` -- CORRECT for melee

**Implementation Notes:**
```
Handler flow:
1. Check player has katar equipped (weaponType === 'katar')
2. If not: emit skill:error "Requires a Katar weapon"
3. Resolve target enemy
4. Range check (melee = 150 + tolerance)
5. Deduct SP, apply delays (ACD = 2000ms)
6. Check for Sonic Acceleration passive: if learned[1106], multiply effectVal by 1.5
7. Check for EDP buff: if active, halve the effective skill multiplier
8. Calculate damage: calculatePhysicalDamage(attacker, target, { isSkill: true, skillMultiplier: effectiveATK% })
9. Apply stun: roll (10 + 2*level)%, check boss immunity
10. Broadcast skill:effect_damage with hits=8 (visual) but single damage value
11. Death check, skill:used, health_update
```

---

### 3. GRIMTOOTH (ID 1102)

| Property | Value |
|----------|-------|
| rAthena ID | 137 |
| Type | Active, Offensive |
| Target | Single target with 3x3 AoE splash |
| Range (cells) | 2+SkillLv (Lv1=3, Lv5=7) |
| Range (UE units) | 150 + SkillLv*50 (Lv1=200, Lv5=350) -- but skill def says 450 |
| Element | Weapon element |
| Weapon Requirement | Katar class ONLY |
| Required Status | Must be in Hiding |
| Cast Time | 0 (instant) |
| After-Cast Delay | ASPD-based |
| Prerequisites | Sonic Blow Lv5, Cloaking Lv2 |
| Critical | Does NOT break Hiding |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |
| SP Cost | 3 | 3 | 3 | 3 | 3 |
| Range (cells) | 3 | 4 | 5 | 6 | 7 |

**Formulas:**
- ATK%: `100 + 20 * SkillLv`
- SP Cost: 3 (flat all levels)
- Range: `2 + SkillLv` cells

**Special Mechanics:**
1. **Must be in Hiding** -- cannot use unless player has `hiding` or `cloaking` buff active (isHidden = true)
2. **Does NOT break Hiding** -- unlike most attacks, Grimtooth keeps the player hidden after use
3. **3x3 AoE splash** -- hits all enemies in a 3x3 area around the primary target
4. **Range classification:**
   - Lv1-2: MELEE attack (blocked by Safety Wall, NOT by Pneuma)
   - Lv3-5: RANGED attack (blocked by Pneuma, NOT by Safety Wall)
5. **Katar required** -- fails without katar
6. **EDP interaction:** EDP does NOT affect Grimtooth damage (confirmed by multiple sources)
7. **Status arrows:** At Lv3+, can proc status effects from status cards, but NOT elemental modifiers from arrows

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists.

**Skill Definition Audit:**
- `range: 450` -- should be per-level: Lv1=150, Lv5=350. But since range varies per level and we only have one range field, use max range 350 and validate per-level in handler. OR: use `range: 350` (max) and let handler reject if target is closer than melee for lv1-2
- `effectValue: 120+i*20` -- CORRECT
- `cooldown: 500` -- should be 0 (delay is ASPD-based, not a fixed cooldown). The ASPD-based delay means the next attack follows normal attack speed timing.
- Prerequisites: `[{ skillId: 1101, level: 5 }, { skillId: 1103, level: 2 }]` = Sonic Blow 5 + Cloaking 2. RMS says Cloaking Lv2 + Sonic Blow Lv5. CORRECT.

**Implementation Notes:**
```
Handler flow:
1. Check player isHidden (getCombinedModifiers)
2. If not hidden: emit skill:error "Must be in Hiding to use Grimtooth"
3. Check katar equipped
4. Calculate per-level range: range = (2 + learnedLevel) * 50 UE units
5. Range check against target using per-level range
6. Deduct SP (3), apply delays (afterCastDelay = ASPD-based or ~500ms fixed)
7. Calculate damage for primary target
8. Find all enemies in 3x3 AoE (radius ~75 UE units) around primary target
9. Apply damage to all targets in AoE
10. Do NOT remove hiding/cloaking buff (critical: Grimtooth stays hidden)
11. Broadcast skill:effect_damage for each hit target
12. Death checks for all hit targets
```

---

### 4. CLOAKING (ID 1103)

| Property | Value |
|----------|-------|
| rAthena ID | 135 |
| Type | Toggle |
| Target | Self |
| Max Level | 10 |
| SP Cost (activation) | 15 |
| Prerequisites | Hiding Lv2 (Thief skill 503) |
| Duration | Until manually canceled, revealed, or SP depleted |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Wall Req | Yes | Yes | No | No | No | No | No | No | No | No |
| Speed (on-wall) | 103% | 106% | 109% | 112% | 115% | 118% | 121% | 124% | 127% | 130% |
| Speed (off-wall) | 0% | 0% | 79% | 82% | 85% | 88% | 91% | 94% | 97% | 100% |
| SP Drain | 1/0.5s | 1/1s | 1/2s | 1/3s | 1/4s | 1/5s | 1/6s | 1/7s | 1/8s | 1/9s |

**SP Drain Intervals (seconds per 1 SP):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Interval | 0.5 | 1.0 | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 7.0 | 8.0 | 9.0 |

**Key Differences from Hiding:**
1. **CAN MOVE** while cloaked (Hiding cannot move at all)
2. **Can use items** without breaking cloak (but using items cancels Cloaking -- needs verification per source)
3. **Skills cancel cloak** -- using any skill other than Cloaking toggle will break it
4. **Incoming skills still execute** -- unlike Hiding, single-target skills that were already cast on you will still hit (Hiding cancels them)
5. **AoE damage still applies** -- ground AoE and splash damage hits cloaked players
6. **Attacks cancel cloak** -- auto-attacking breaks Cloaking
7. **Level 1-2 require wall adjacency** -- must be next to a wall to stay cloaked
8. **No HP/SP regen** while cloaked
9. **Movement speed changes** -- speed varies by level and wall proximity

**Detection Methods (same as Hiding):**
- Sight, Ruwach, Detecting skills
- Insect race, Demon race, Boss Protocol monsters
- Maya Purple card (headgear)
- Improve Concentration (reveals hidden in AoE)
- AoE damage skills (Heaven's Drive, Thunderstorm, Arrow Shower, etc.)

**Break Conditions:**
1. Manual toggle (use Cloaking again)
2. SP reaches 0
3. Detected by reveal skill/monster
4. Using a skill (auto-cancels)
5. Auto-attacking (auto-cancels)
6. Lv1-2: Moving away from wall

**Status:** HANDLER NOT IMPLEMENTED. Buff entry exists in `ro_buff_system.js`.

**Skill Definition Audit:**
- `type: 'toggle'` -- CORRECT
- `spCost: 15` -- CORRECT
- `effectValue: 65+i*3` -- meaning unclear, not used in handler yet. Should store movement speed data.
- Prerequisites: `[{ skillId: 503, level: 2 }]` = Hiding Lv2. CORRECT.

**Implementation Notes:**
```
Cloaking is essentially "Hiding but you can move" with different SP drain and wall mechanics.

Handler flow (toggle ON):
1. If already cloaked: remove cloaking buff, broadcast buff_removed, return (toggle OFF)
2. Deduct 15 SP
3. Apply cloaking buff with: { skillLevel, lastDrainTime: now, wallRequired: level <= 2 }
4. Set isHidden = true (same flag as Hiding)
5. Broadcast skill:buff_applied to zone
6. Emit skill:used

SP drain tick (piggyback on existing buff tick):
- Same system as Hiding SP drain but with different intervals (0.5s to 9s)
- Interval formula: Level 1=0.5s, Level 2=1s, Level 3+=level-1 seconds
  Actually the pattern is: [0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9] -- hardcode as lookup table

Wall check (for Lv1-2):
- Server needs wall proximity data. For our MMO, we can skip wall requirement entirely
  OR implement a simplified version where Lv1-2 simply have much higher SP drain (already reflected in the 0.5s/1s drain rates)
- RECOMMENDED: Skip wall checks for initial implementation. The high SP drain at Lv1-2 naturally limits usage.

Movement speed:
- Movement speed bonuses are cosmetic client-side for now
- Server doesn't enforce movement speed (position broadcast is trusted)
- Can add speed modifier to buff payload for future client implementation

Key distinction from Hiding:
- Cloaking allows position updates (no movement block)
- Hiding blocks position updates
```

---

### 5. POISON REACT (ID 1104)

| Property | Value |
|----------|-------|
| rAthena ID | 139 |
| Type | Active, Self-Buff |
| Target | Self |
| Max Level | 10 |
| Prerequisites | Enchant Poison Lv3 (our ID: 1109) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Counter ATK% | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 370 | 400 |
| Envenom Counters | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 6 |
| Duration (s) | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 60 |
| SP Cost | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 45 | 45 |

**Formulas:**
- Counter ATK%: `100 + 30 * SkillLv`
- Envenom Counter Limit: `floor((SkillLv + 1) / 2)` for Lv1-9, 6 for Lv10
- Duration: `15 + 5 * SkillLv` seconds (capped at 60s for Lv9-10)
- SP Cost: varies per level (not a clean formula)

**Two Modes of Operation:**

**Mode A -- Poison Element Counter (one-time):**
- When hit by a Poison-property attack, automatically counterattacks with `(100 + 30*Lv)%` ATK damage
- 50% chance to inflict Poison status on the attacker
- This consumes the buff entirely (one-shot counter)
- Only triggers once per Poison React activation

**Mode B -- Non-Poison Envenom Counter (multi-use):**
- When hit by any NON-poison physical attack, 50% chance to auto-cast Envenom Lv5
- This can trigger up to `Envenom Counter Limit` times
- After all counters are used, the buff expires
- Does NOT consume the poison counter (Mode A still available until used)

**Buff Expires When:**
1. Duration runs out
2. Poison counter used (Mode A triggers)
3. All Envenom counters used (Mode B limit reached)
4. Whichever comes first

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists but has issues.

**Skill Definition Audit:**
- `spCost: 10` -- WRONG. SP varies: 25/30/35/40/45/50/55/60/45/45. Need per-level SP.
- `effectValue: 50` -- meaning unclear. Should be counter ATK% (130-400) or stored separately.
- `duration: 20000+i*10000` -- WRONG. Duration is 20/25/30/35/40/45/50/55/60/60s. Formula should be `(15+5*(i+1))*1000` capped at 60000.
- Prerequisites: `[{ skillId: 504, level: 3 }]` = Envenom Lv3. Should be Enchant Poison Lv3 (1109, not 504). WRONG prerequisite skill ID.

**Implementation Notes:**
```
This is a REACTIVE BUFF -- it requires a new system to hook into incoming damage events.

Handler flow (activation):
1. Deduct SP, apply delays
2. Apply poison_react buff with:
   { skillLevel, counterATK: 100+30*level, envenomLimit: floor((level+1)/2) or 6 at lv10,
     envenomUsed: 0, poisonCounterUsed: false, duration }
3. Broadcast buff_applied

Damage hook (in combat tick or auto-attack damage path):
When player with active poison_react buff takes physical damage:
1. Check if attacker's attack element is Poison:
   YES -> Mode A: counter with (100+30*Lv)% ATK, 50% poison chance, remove buff
   NO  -> Mode B: 50% chance auto-cast Envenom Lv5, increment envenomUsed
          If envenomUsed >= envenomLimit: remove buff
```

---

### 6. VENOM DUST (ID 1105)

| Property | Value |
|----------|-------|
| rAthena ID | 140 |
| Type | Active, Ground AoE |
| Target | Ground |
| AoE Size | 2x2 cells (some sources say 3x3 for effect check) |
| Range | 2 cells (~100 UE units) |
| Element | Poison |
| Catalyst | 1 Red Gemstone per cast |
| Max Level | 10 |
| Prerequisites | Enchant Poison Lv5, Poison React Lv3 -- NOTE: current prereqs are wrong |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| SP Cost | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |

**Formula:**
- Duration: `5 * SkillLv` seconds
- SP Cost: 20 (flat)

**Mechanics:**
1. Places a poison cloud on the ground at the targeted location
2. Any enemy standing in the cloud gets Poison status applied
3. Does NOT deal direct damage -- only applies poison status effect
4. Does NOT poison Boss-type monsters
5. Does NOT refresh poison duration on already-poisoned enemies
6. Cloud persists for the full duration even if caster moves away or dies
7. Requires 1 Red Gemstone per cast (consumed)
8. Ground-targeted skill (requires `groundX/Y/Z`)

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists but has prerequisite issues.

**Skill Definition Audit:**
- `range: 450` -- WRONG. Should be ~100 (2 cells). 450 is way too far.
- `spCost: 20` -- CORRECT
- `effectValue: 0` -- CORRECT (no damage)
- `duration: 5000+i*5000` -- CORRECT
- Prerequisites: `[{ skillId: 504, level: 5 }, { skillId: 1104, level: 3 }]` = Envenom Lv5 + Poison React Lv3. Should be Enchant Poison Lv5 (1109). WRONG prerequisite.

**Implementation Notes:**
```
This requires the GROUND EFFECT system -- a persistent zone on the map that applies effects.

Handler flow:
1. Check Red Gemstone in inventory (SKILL_CATALYSTS entry needed)
2. Consume 1 Red Gemstone
3. Deduct SP
4. Create ground effect entry: { position, radius: 50 (2x2), duration, effect: 'poison' }
5. Store in activeGroundEffects Map
6. Broadcast to zone: ground effect placed

Ground effect tick (piggyback on existing tick):
Every 1 second:
- For each active ground effect:
  - Check all enemies in radius
  - Apply Poison status to non-boss, non-poisoned enemies
  - Remove expired ground effects
```

---

### 7. SONIC ACCELERATION (ID 1106)

| Property | Value |
|----------|-------|
| rAthena ID | 1003 |
| Type | Passive (Quest Skill) |
| Max Level | 1 |
| Quest Requirement | Assassin Skill Quest (Job Level 30+) |
| Weapon | Katar |

**Effect:**
- +50% damage to Sonic Blow (multiplicative)
- +50% HIT rate for Sonic Blow (multiplicative -- applied to final hit rate, not raw HIT stat)

**Pre-Renewal Canonical Values:**
- Some sources report +10% damage / +50% HIT (older data)
- Other sources report +90% damage / +90% HIT (post-2023 balance patch)
- For our implementation: Use classic pre-renewal values of **+50% damage, +50% HIT**

**Status:** DEFINED but passive logic not yet in `getPassiveSkillBonuses()`. Needs to be read during Sonic Blow handler execution.

**Skill Definition Audit:**
- `effectValue: 50` -- CORRECT (50% bonus)
- `questSkill: true` -- CORRECT

**Implementation Notes:**
```
NOT handled in getPassiveSkillBonuses() because it only affects one specific skill.
Instead, check learned[1106] inside the Sonic Blow handler:
  if (player.learnedSkills[1106]) {
      effectiveMultiplier = Math.floor(effectVal * 1.5);
      // Also multiply final hit rate by 1.5
  }
```

---

### 8. RIGHTHAND MASTERY (ID 1107)

| Property | Value |
|----------|-------|
| rAthena ID | 132 |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | None |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Right Hand ATK% | 60% | 70% | 80% | 90% | 100% |

**Mechanics:**
- Recovers the dual-wield right-hand damage penalty
- Base penalty without mastery: right hand deals only 50% damage
- Formula: right hand effective = `50 + RHM_Lv * 10`% (Lv5 = 100% = full damage)
- **Only affects auto-attacks, NOT skills** (skills always use full weapon damage)
- Has no effect when using a Katar or single weapon

**Status:** IMPLEMENTED in `getPassiveSkillBonuses()` and used in combat tick dual wield logic.

**Skill Definition Audit:** CORRECT. No changes needed.

---

### 9. LEFTHAND MASTERY (ID 1108)

| Property | Value |
|----------|-------|
| rAthena ID | 133 |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Righthand Mastery Lv2 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Left Hand ATK% | 40% | 50% | 60% | 70% | 80% |

**Mechanics:**
- Recovers the dual-wield left-hand damage penalty
- Base penalty without mastery: left hand deals only 30% damage
- Formula: left hand effective = `30 + LHM_Lv * 10`% (Lv5 = 80% = never fully recovered)
- **Only affects auto-attacks, NOT skills**
- Left hand always force-hits (cannot miss) and never crits independently

**Status:** IMPLEMENTED in `getPassiveSkillBonuses()` and used in combat tick dual wield logic.

**Skill Definition Audit:** CORRECT. No changes needed.

---

### 10. ENCHANT POISON (ID 1109)

| Property | Value |
|----------|-------|
| rAthena ID | 138 |
| Type | Supportive (Buff) |
| Target | Self or Party Member (1 cell range for party members) |
| Max Level | 10 |
| Cast Time | 0 |
| After-Cast Delay | 1 second |
| Prerequisites | Envenom Lv1 (Thief skill 504) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 30 | 45 | 60 | 75 | 90 | 105 | 120 | 135 | 150 | 165 |
| Poison Chance | 3.0% | 3.5% | 4.0% | 4.5% | 5.0% | 5.5% | 6.0% | 6.5% | 7.0% | 7.5% |
| SP Cost | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |

**Formulas:**
- Duration: `15 + 15 * SkillLv` seconds
- Poison Chance: `2.5 + 0.5 * SkillLv` %
- SP Cost: 20 (flat)

**Mechanics:**
1. Changes the weapon's element to **Poison** for the duration
2. Each physical attack has a `(2.5 + 0.5*Lv)%` chance to inflict Poison status on the target
3. **Switching weapons cancels the buff**
4. Can be used on self or friendly players (1 cell range for party members)
5. Does NOT stack with other weapon element endows (Aspersio, Sage endows, etc.) -- last one applied wins
6. The poison status from this is the regular Poison (not Deadly Poison)

**Buff Implementation:**
- Apply buff `enchant_poison` with `weaponElement: 'poison'`
- `getCombinedModifiers()` already handles `weaponElement` for element endow buffs
- Need to add poison proc check in auto-attack damage path

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists.

**Skill Definition Audit:**
- `targetType: 'single'` -- should be `'self'` for self-cast, or keep `'single'` to allow targeting party members. RO Classic allows targeting self + party. Keep as `'single'` (self-target falls back to caster).
- `range: 150` -- CORRECT for party member targeting
- `spCost: 20` -- CORRECT
- `effectValue: 25+i*5` -- this encodes poison chance as 25/30/35.../70/75 (need to divide by 10 to get 2.5/3.0/.../7.5%). OR treat as tenths. Current values encode 25-75 which represents 2.5%-7.5% as integer tenths.
- `duration: 30000+i*15000` -- CORRECT (30/45/60/75/90/105/120/135/150/165 seconds)
- Prerequisites: `[{ skillId: 504, level: 1 }]` = Envenom Lv1. CORRECT.

**Implementation Notes:**
```
Handler flow:
1. Resolve target (self if no targetId, or friendly player)
2. Deduct SP, apply delays (ACD = 1000ms)
3. Remove any existing weapon element buff on target (Aspersio, etc.)
4. Apply enchant_poison buff: {
     weaponElement: 'poison',
     poisonChance: (25 + 5*level) / 10, // 2.5-7.5%
     duration: (15 + 15*level) * 1000
   }
5. Broadcast buff_applied
6. In auto-attack damage path: if attacker has enchant_poison buff,
   roll poisonChance% to apply Poison status on target

Weapon switch hook:
When player equips a different weapon, remove enchant_poison buff
(already exists for weapon element endows if implemented)
```

---

### 11. VENOM SPLASHER (ID 1110)

| Property | Value |
|----------|-------|
| rAthena ID | 141 |
| Type | Active, Offensive (Delayed) |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| AoE on Detonation | 5x5 cells |
| Element | Poison |
| Catalyst | 1 Red Gemstone (pre-renewal) |
| Max Level | 10 |
| Prerequisites | Poison React Lv5, Venom Dust Lv5 |
| Cast Time | 1 second |

**Pre-Renewal Data (RateMyServer):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 550 | 600 | 650 | 700 | 750 | 800 | 850 | 900 | 950 | 1000 |
| SP Cost | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Timer (s) | 5 | 5.5 | 6 | 6.5 | 7 | 7.5 | 8 | 8.5 | 9 | 9.5 |
| Cooldown (s) | 7.5 | 8 | 8.5 | 9 | 9.5 | 10 | 10.5 | 11 | 11.5 | 12 |

**Formulas:**
- ATK%: `500 + 50 * SkillLv`
- SP Cost: `10 + 2 * SkillLv`
- Explosion Timer: `4.5 + 0.5 * SkillLv` seconds (time before detonation)
- Cooldown: `7 + 0.5 * SkillLv` seconds

**Mechanics:**
1. **Target HP Restriction:** Target must be at or below 75% HP (3/4 of MaxHP)
2. **Timed Explosion:** After casting, the target is "marked" and a timer starts
3. After the timer expires, the target EXPLODES dealing damage in a 5x5 AoE
4. The explosion damage is applied to ALL enemies in the 5x5 AoE (including the marked target)
5. **Cannot target Boss monsters** (Boss Protocol immunity)
6. **Ignores FLEE** -- the explosion always hits (forceHit on detonation)
7. Consumes 1 Red Gemstone per cast
8. **If target dies before explosion:** explosion is canceled
9. **If target hides before explosion:** damage canceled but poison chance remains
10. **Cooldown** is per-skill and separate from after-cast delay
11. **Only one Venom Splasher can be active at a time** per caster

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists but has issues.

**Skill Definition Audit:**
- `spCost: 12+i*2` -- CORRECT
- `castTime: 1000` -- CORRECT (1 second cast time)
- `cooldown: 0` -- WRONG. Should be per-level cooldown (7000 + level*500)
- `effectValue: 500+i*100` -- CLOSE but should be `500+50*(i+1)` = 550/600/.../1000. Currently gives 500/600/.../1400. WRONG -- effectValue should use `500+50*(i+1)` formula. Wait, `500+i*100` = 500/600/700/800/900/1000/1100/1200/1300/1400. RMS says 550/600/650/700/750/800/850/900/950/1000. The current formula is WRONG.
- `duration: 2000+i*1000` -- this was being used for explosion timer but is also wrong. Timer should be 5000/5500/6000/.../9500.
- Prerequisites: `[{ skillId: 1104, level: 5 }, { skillId: 1105, level: 5 }]` = Poison React 5 + Venom Dust 5. CORRECT per RMS.

**Implementation Notes:**
```
This requires a DELAYED DETONATION system.

Handler flow:
1. Check Red Gemstone in inventory
2. Check target HP <= 75% MaxHP
3. Check target is not Boss
4. Check no other Venom Splasher active from this caster
5. Consume Red Gemstone, deduct SP
6. Cast time: 1 second (enters activeCasts)
7. On cast complete:
   a. Mark target with venomSplasherMark: { casterId, attackerStats, level, effectVal, detonateAt: now + timer }
   b. Start detonation timer: setTimeout or track in tick loop
   c. Apply per-skill cooldown
8. On detonation:
   a. If target is dead or hidden: cancel
   b. Calculate damage using effectVal% ATK (forceHit, Poison element)
   c. Find all enemies in 5x5 AoE (radius ~125 UE units) around target
   d. Apply damage to all enemies in AoE
   e. Broadcast skill:effect_damage for each hit
   f. Death checks

Storage: venomSplasherMarks Map<enemyId, markData>
Tick check: in combat tick, check if any marks have reached detonateAt
```

---

### 12. THROW VENOM KNIFE (ID 1111)

| Property | Value |
|----------|-------|
| rAthena ID | 1004 |
| Type | Active, Offensive (Quest Skill) |
| Target | Single Enemy |
| Range | 9 cells (~450 UE units) -- some sources say 10 cells |
| Element | Weapon element (right hand) |
| Max Level | 1 |
| Quest Requirement | Assassin Skill Quest |

**Pre-Renewal Data (RateMyServer):**

| Property | Value |
|----------|-------|
| ATK% | 500% (post-2023 buff, was 100% originally) |
| SP Cost | 35 |
| Poison Chance | Based on Envenom level |
| Ammunition | 1 Venom Knife consumed per cast |

**Mechanics:**
1. Ranged physical attack
2. Damage derived from base ATK only (unaffected by % damage bonuses -- only flat ATK bonuses apply)
3. Consumes 1 Venom Knife ammunition per use (purchasable from Assassin Guild, 50z each)
4. Poison chance based on Envenom skill level: `(10 + 4 * EnvenomLv)%`
5. EDP does NOT affect this skill
6. Takes weapon element from right hand
7. Ice Pick effect works with this skill

**Status:** HANDLER NOT IMPLEMENTED. Skill definition exists but has issues.

**Skill Definition Audit:**
- `range: 500` -- CLOSE. Should be 450 (9 cells * 50 UE) or 500 (10 cells * 50 UE). Minor.
- `spCost: 15` -- WRONG. Should be 35.
- `effectValue: 100` -- OUTDATED. Post-balance is 500%. For pre-renewal classic, use 100% (original value). Decision: use 100% for authenticity.
- `element: 'poison'` -- WRONG. Should use weapon element. Set to `'neutral'` and let handler use weapon element.

**Implementation Notes:**
```
Handler flow:
1. Check Venom Knife in inventory
2. If not: emit skill:error "You need Venom Knives"
3. Consume 1 Venom Knife
4. Deduct SP (35)
5. Calculate damage: base ATK only (no % modifiers apply -- only flat bonuses)
   This is unusual -- need to pass a special flag to damage calc
6. Apply poison chance based on Envenom level
7. Broadcast, death check, etc.

Note: The "base ATK only" mechanic makes this unique. For simplicity,
can use calculatePhysicalDamage with skillMultiplier=100 (normal hit)
and just not apply any extra skill multiplier bonuses.
```

---

## Assassin Cross Skills (Transcendent)

Assassin Cross inherits ALL Thief + Assassin skills and gains 5 new skills. These are defined in the `assassin_cross` class ID range.

### 13. ADVANCED KATAR MASTERY

| Property | Value |
|----------|-------|
| rAthena ID | 376 |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | (High Thief) Double Attack Lv5, (Assassin Cross) Katar Mastery Lv7 |
| Weapon | Katar class only |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Katar ATK +% | 12% | 14% | 16% | 18% | 20% |

**Mechanics:**
- Multiplies ALL physical damage by `(100 + bonus)%` when using a Katar
- This is a % modifier, unlike Katar Mastery which is flat
- Applied after base damage calculation, before DEF

**Implementation Notes:**
```
In getPassiveSkillBonuses():
  const akmLv = learned[AKM_ID] || 0;
  if (akmLv > 0 && wType === 'katar') {
      bonuses.katarDamagePercent = 10 + akmLv * 2; // 12/14/16/18/20%
  }

In damage calculation:
  if (attacker.katarDamagePercent) {
      totalATK = Math.floor(totalATK * (100 + attacker.katarDamagePercent) / 100);
  }
```

---

### 14. CREATE DEADLY POISON

| Property | Value |
|----------|-------|
| rAthena ID | 407 |
| Type | Active (Crafting) |
| Target | Self |
| Max Level | 1 |
| SP Cost | 50 |
| After-Cast Delay | 5 seconds |
| Prerequisites | (High Thief) Envenom Lv10, Detoxify Lv1. (Assassin Cross) Enchant Poison Lv5 |

**Success Formula:**
`Success_Rate(%) = 20 + (DEX * 0.4) + (LUK * 0.2)`

**Materials Required (ALL consumed on attempt):**
1. Empty Bottle x1
2. Berserk Potion x1
3. Karvodainirol x1
4. Poison Spore x1
5. Bee Stinger x1
6. Cactus Needle x1
7. Venom Canine x1

**Result on Success:** Creates 1 Poison Bottle (used as catalyst for EDP)

**Result on Failure:** Caster takes 20% of MaxHP as damage (self-damage, cannot kill)

**Implementation Notes:**
```
Handler flow:
1. Check all 7 materials in inventory
2. If any missing: emit skill:error listing missing items
3. Consume all 7 materials
4. Deduct 50 SP
5. Apply 5s after-cast delay
6. Roll success: 20 + DEX*0.4 + LUK*0.2
7. If success:
   - Add 1 Poison Bottle to inventory
   - Emit success message
8. If failure:
   - Deal 20% MaxHP to caster (cannot kill: min HP = 1)
   - Emit failure message
   - Broadcast health_update
```

---

### 15. ENCHANT DEADLY POISON (EDP)

| Property | Value |
|----------|-------|
| rAthena ID | 378 |
| Type | Supportive (Self-Buff) |
| Target | Self |
| Max Level | 5 |
| Catalyst | 1 Poison Bottle per cast |
| After-Cast Delay | 2 seconds |
| Prerequisites | Create Deadly Poison Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK Multiplier | 2.0x | 2.5x | 3.0x | 3.5x | 4.0x |
| Duration (s) | 40 | 45 | 50 | 55 | 60 |
| SP Cost | 60 | 70 | 80 | 90 | 100 |

**Formulas:**
- ATK Multiplier: `1.5 + 0.5 * SkillLv` (weapon ATK portion only)
- Duration: `35 + 5 * SkillLv` seconds
- SP Cost: `50 + 10 * SkillLv`

**Pre-Renewal Mechanics:**
1. Multiplies weapon ATK portion of damage by the multiplier
2. **Does NOT change weapon element to Poison** -- weapon keeps its current element
3. Each physical hit has a chance to inflict **Deadly Poison** (not regular Poison)
   - Deadly Poison is more severe: faster HP drain, cannot be cured by regular antidote
   - Deadly Poison HP drain cannot reduce HP below 25%
4. **Cannot be Dispelled**
5. **Canceled on weapon switch**

**Skill Modifier Halving (Pre-Renewal):**
When EDP is active, certain skills have their skill modifier HALVED before EDP multiplier is applied:
- Sonic Blow: `effectiveATK% = SB_ATK% / 2`
- Soul Breaker (physical portion): halved

**Skills NOT affected by EDP:**
- Grimtooth
- Venom Knife
- Meteor Assault
- Soul Destroyer (confirmed NOT affected in pre-renewal)

**EDP Damage Formula (Pre-Renewal):**
```
For normal auto-attacks:
  damage = normalATK * edpMultiplier

For Sonic Blow:
  damage = normalATK * (SB_ATK% / 2 / 100) * edpMultiplier
  e.g., SB Lv10 + EDP Lv5 = normalATK * (800/2/100) * 4.0 = normalATK * 16.0

For skills NOT affected:
  damage = normalATK * (skillATK% / 100)  [EDP ignored entirely]
```

**Implementation Notes:**
```
Handler flow:
1. Check Poison Bottle in inventory
2. Consume 1 Poison Bottle
3. Deduct SP, apply 2s ACD
4. Apply edp buff: {
     atkMultiplier: 1.5 + 0.5*level,
     deadlyPoisonChance: TBD (probably same as Enchant Poison rates),
     duration
   }
5. Broadcast buff_applied

In damage calculation:
- Check for EDP buff on attacker
- For auto-attacks: multiply weapon ATK by edpMultiplier
- For Sonic Blow: halve skill multiplier, then multiply by edpMultiplier
- For immune skills (Grimtooth, Venom Knife, Meteor Assault): skip EDP entirely
```

---

### 16. SOUL DESTROYER (Soul Breaker)

| Property | Value |
|----------|-------|
| rAthena ID | 379 |
| Type | Offensive, Ranged |
| Target | Single Enemy |
| Range | 9 cells (450 UE units) -- some sources say 10 cells |
| Element | Weapon element (right hand) |
| Max Level | 10 |
| Cast Time | 0.5-0.7 seconds (variable) |
| Prerequisites | (High Thief) Double Attack Lv5, Envenom Lv5. (SinX) Cloaking Lv3, Enchant Poison Lv6 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 150 | 300 | 450 | 600 | 750 | 900 | 1050 | 1200 | 1350 | 1500 |
| SP Cost | 20 | 20 | 20 | 20 | 20 | 30 | 30 | 30 | 30 | 30 |
| Cast Delay (s) | 1.0 | 1.2 | 1.4 | 1.6 | 1.8 | 2.0 | 2.2 | 2.4 | 2.6 | 2.8 |

**Formulas:**
- Physical ATK%: `150 * SkillLv`
- SP: 20 for Lv1-5, 30 for Lv6-10
- Cast Delay (ACD): `0.8 + 0.2 * SkillLv` seconds

**Pre-Renewal Damage Formula (UNIQUE -- hybrid physical+magical):**

This skill has TWO independent damage components that are added together:

**Component 1: Physical ATK (subject to HIT/FLEE, DEF, elements):**
```
PhysicalDamage = (TotalATK - LeftHandWeaponATK) * SkillLv
```
- Uses right-hand weapon only (left-hand ATK excluded)
- Subject to HIT/FLEE check (can miss)
- Subject to target DEF (hard DEF + soft DEF)
- Respects elemental modifiers (weapon element vs target element)
- Card modifiers apply (race%, element%, size%, etc.)
- Ice Pick effect works (ignores DEF based on target VIT)

**Component 2: INT (always hits, ignores MDEF, non-elemental):**
```
INTDamage = (INT * 5 * SkillLv) + random(500, 1000)
```
- ALWAYS hits (ignores FLEE entirely)
- NOT affected by MDEF
- Non-elemental (100% damage to all elements including Ghost)
- NOT affected by Maya Card, % damage cards
- IS affected by Raydric Card, Thara Frog, Horn, damage reduction effects

**Final Damage:**
```
FinalDamage = PhysicalDamage + INTDamage
```

**Special Mechanics:**
1. **Ranged attack** -- blocked by Pneuma, NOT by Safety Wall
2. **EDP does NOT affect** Soul Destroyer damage
3. **Left-hand weapon ATK irrelevant** -- only right hand used
4. **Cast time reducible** by DEX (standard pre-renewal formula)
5. **Can trigger critical** on the physical component (half crit rate, half crit bonus)

**Status:** NOT DEFINED. Needs new skill definition entry.

**Implementation Notes:**
```
This needs a CUSTOM damage calculation because of the hybrid formula.

Handler flow:
1. Resolve target, range check (450 UE)
2. Cast time check (0.5s base, reducible by DEX)
3. On cast complete:
   a. Deduct SP
   b. Apply ACD (level-dependent)
   c. Calculate Physical component:
      - Remove left-hand ATK from total
      - Apply HIT/FLEE, DEF, elements, cards
      - Multiply by SkillLv
   d. Calculate INT component:
      - INT * 5 * SkillLv + random(500, 1000)
      - No DEF, no element, always hits
   e. Total = Physical + INT
   f. Broadcast skill:effect_damage
   g. Death check

Special damage calc function needed:
  calculateSoulDestroyerDamage(attacker, target, skillLevel)
```

---

### 17. METEOR ASSAULT

| Property | Value |
|----------|-------|
| rAthena ID | 406 |
| Type | Offensive, Self-Centered AoE |
| Target | All enemies within 5x5 cells around caster |
| Range | Self-centered |
| Max Level | 10 |
| Cast Time | 0.5 seconds (fixed in pre-renewal? some sources say variable) |
| After-Cast Delay | 0.5 seconds |
| Prerequisites | Katar Mastery Lv5, Righthand Mastery Lv3, Sonic Blow Lv5, Soul Destroyer Lv1 |

**Pre-Renewal Data (RateMyServer -- confirmed pre-RE values):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 80 | 120 | 160 | 200 | 240 | 280 | 320 | 360 | 400 | 440 |
| Status% | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 |
| SP Cost | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 |

**Formulas (Pre-Renewal):**
- ATK%: `40 + 40 * SkillLv`
- Status Chance: `5 + 5 * SkillLv` %
- SP Cost: `8 + 2 * SkillLv`

**NOTE:** The renewal version has MUCH higher values (320-1400% with BaseLv scaling). The pre-renewal values from RateMyServer Pre-RE DB are significantly lower (80-440%).

**Status Effects Applied (each rolled independently):**
- Stun (duration: standard stun)
- Blind (duration: standard blind)
- Bleeding (duration: standard bleeding)
- Each status has an independent `(5 + 5*Lv)%` chance

**Special Mechanics:**
1. **Unaffected by EDP** -- EDP multiplier does NOT apply
2. **No weapon requirement** -- works with any weapon (not katar-only)
3. **Self-centered** -- targetType is 'aoe', hits everything around the caster
4. **5x5 AoE** -- radius of ~125 UE units (2.5 cells in each direction)
5. **Physical damage** -- uses weapon element, subject to DEF

**Status:** NOT DEFINED. Needs new skill definition entry.

**Implementation Notes:**
```
Handler flow:
1. Cast time: 500ms (reducible by DEX)
2. On cast complete:
   a. Deduct SP
   b. Apply 500ms ACD
   c. Find all enemies in 5x5 AoE (radius 125) around caster position
   d. For each enemy in range:
      - Calculate physical damage with effectVal% multiplier
      - Roll stun: (5+5*level)%
      - Roll blind: (5+5*level)%
      - Roll bleed: (5+5*level)%
      - Apply damage + status effects
      - Broadcast skill:effect_damage per target
   e. Death checks for all
   f. skill:used, health_update
```

---

## Skill Tree & Prerequisites

### Thief -> Assassin Prerequisite Chain

```
THIEF TREE (inherited):
  Double Attack (500)  -----> [no prereqs, required for SinX: Lv5]
  Improve Dodge (501)  -----> [no prereqs]
  Steal (502)          -----> [no prereqs]
  Hiding (503)         -----> [Steal Lv5, required for: Cloaking Lv2]
  Envenom (504)        -----> [no prereqs, required for: Enchant Poison]
  Detoxify (505)       -----> [Envenom Lv3]
  Sand Attack (506)    -----> [quest]
  Back Slide (507)     -----> [quest]
  Pick Stone (508)     -----> [quest]
  Throw Stone (509)    -----> [Pick Stone Lv1, quest]

ASSASSIN TREE:
  Katar Mastery (1100) -----> [no prereqs]
      |
      +-- Sonic Blow (1101) -----> [Katar Mastery Lv4]
      |       |
      |       +-- Grimtooth (1102) -----> [Sonic Blow Lv5, Cloaking Lv2]
      |
  Righthand Mastery (1107) -----> [no prereqs]
      |
      +-- Lefthand Mastery (1108) -----> [Righthand Mastery Lv2]
      |
  Cloaking (1103) -----> [Hiding Lv2 (Thief)]
      |
  Enchant Poison (1109) -----> [Envenom Lv1 (Thief)]
      |
      +-- Poison React (1104) -----> [Enchant Poison Lv3]
              |
              +-- Venom Dust (1105) -----> [Enchant Poison Lv5, Poison React Lv3]
              |       |
              |       +-- Venom Splasher (1110) -----> [Poison React Lv5, Venom Dust Lv5]
              |
  Sonic Acceleration (1106) -----> [Quest, Job Lv30+]
  Throw Venom Knife (1111) -----> [Quest, Job Lv30+]
```

### Assassin -> Assassin Cross Prerequisite Chain

```
ASSASSIN CROSS TREE (transcendent, inherits all above):
  Advanced Katar Mastery -----> [(High Thief) Double Attack Lv5, (SinX) Katar Mastery Lv7]
      |
  Create Deadly Poison -----> [(High Thief) Envenom Lv10, Detoxify Lv1. (SinX) Enchant Poison Lv5]
      |
      +-- Enchant Deadly Poison -----> [Create Deadly Poison Lv1]
      |
  Soul Destroyer -----> [(High Thief) Double Attack Lv5, Envenom Lv5. (SinX) Cloaking Lv3, Enchant Poison Lv6]
      |
  Meteor Assault -----> [Katar Mastery Lv5, Righthand Mastery Lv3, Sonic Blow Lv5, Soul Destroyer Lv1]
```

---

## Existing Skill Definition Audit

Summary of issues found in `server/src/ro_skill_data_2nd.js` for Assassin skills (IDs 1100-1111):

| ID | Skill | Field | Current | Correct | Priority |
|----|-------|-------|---------|---------|----------|
| 1101 | Sonic Blow | spCost | 16 (flat) | `14 + 2*(i+1)` (16/18/20/.../34) | HIGH |
| 1101 | Sonic Blow | cooldown | 2000 | Should be afterCastDelay: 2000, cooldown: 0 | MEDIUM |
| 1102 | Grimtooth | range | 450 | Per-level: use 350 (max) and validate in handler | LOW |
| 1102 | Grimtooth | cooldown | 500 | 0 (ASPD-based, not fixed cooldown) | LOW |
| 1104 | Poison React | spCost | 10 (flat) | Per-level: 25/30/35/40/45/50/55/60/45/45 | HIGH |
| 1104 | Poison React | effectValue | 50 (flat) | Counter ATK%: `100+30*(i+1)` (130-400) | HIGH |
| 1104 | Poison React | duration | `20000+i*10000` | `(15+5*(i+1))*1000` capped at 60000 | MEDIUM |
| 1104 | Poison React | prerequisites | `[{504, 3}]` | `[{1109, 3}]` (Enchant Poison, not Envenom) | HIGH |
| 1105 | Venom Dust | range | 450 | 100 (2 cells) | MEDIUM |
| 1105 | Venom Dust | prerequisites | `[{504,5},{1104,3}]` | `[{1109,5},{1104,3}]` (EP not Envenom) | HIGH |
| 1110 | Venom Splasher | effectValue | `500+i*100` (500-1400) | `500+50*(i+1)` (550-1000) | HIGH |
| 1110 | Venom Splasher | cooldown | 0 | `7000+500*(i+1)` (7500-12000) | MEDIUM |
| 1110 | Venom Splasher | duration | `2000+i*1000` | `(4.5+0.5*(i+1))*1000` (5000-9500) | MEDIUM |
| 1111 | Throw Venom Knife | spCost | 15 | 35 | MEDIUM |
| 1111 | Throw Venom Knife | element | 'poison' | 'neutral' (uses weapon element at runtime) | LOW |

---

## New Systems Required

### System 1: Cloaking Toggle (extends Hiding system)

**Scope:** New toggle handler in `skill:use` for Cloaking

**What exists:** Hiding toggle already implemented for Thief. Cloaking shares the `isHidden = true` flag. Buff entry exists in `ro_buff_system.js`.

**What's new:**
- Cloaking allows movement (Hiding does not)
- Different SP drain intervals (0.5s-9s vs Hiding's 5s-14s)
- No wall checks (simplified for our implementation)
- Movement speed modifier (cosmetic for now)

**Implementation:**
- Reuse Hiding's `isHidden` flag and detection system
- Add Cloaking-specific SP drain intervals as a lookup table
- Allow position updates while cloaked (do NOT reject movement)
- Use same reveal/detection hooks as Hiding

**Effort:** LOW (mostly reuse Hiding infrastructure)

---

### System 2: Poison React Damage Hook

**Scope:** Reactive counter-attack system triggered by incoming damage

**What exists:** Auto-attack damage path processes hits sequentially in the combat tick. Buff system tracks `poison_react` buff.

**What's new:**
- Hook into the damage application path (combat tick + skill damage)
- When player with `poison_react` buff takes physical damage:
  - Check if attack is Poison element -> Mode A counter
  - Else -> 50% chance Mode B Envenom auto-cast
- Track counter usage (`envenomUsed`, `poisonCounterUsed`)
- Remove buff when counters exhausted

**Implementation:**
- Add a `processPoisonReact(defender, attacker, attackElement)` function
- Call it from: combat tick (auto-attack damage), `skill:effect_damage` handler
- Mode A: full `executePhysicalSkillOnEnemy` counter-attack with (100+30*Lv)% ATK
- Mode B: call existing Envenom logic at Lv5

**Effort:** MEDIUM (new damage hook, two counter modes)

---

### System 3: Ground Effect System (Venom Dust)

**Scope:** Persistent ground zones that apply effects to enemies standing in them

**What exists:** No ground effect system exists. Fire Wall is the closest analog but doesn't persist as a zone.

**What's new:**
- `activeGroundEffects` Map tracking all active ground zones
- Each zone: `{ id, position, radius, effect, duration, startTime, zoneId }`
- Ground effect tick (can piggyback on existing 1s tick):
  - For each ground effect: check all enemies in radius
  - Apply effect (Poison for Venom Dust)
  - Remove expired effects
- Broadcast ground effect placement/removal for client VFX

**Integration:**
- Safety Wall uses a similar concept (block melee in an area)
- Fire Wall has positioned damage zones
- Can build a generic system reusable for future ground skills (Quagmire, Land Protector, etc.)

**Effort:** MEDIUM (new system, but reusable for many future skills)

---

### System 4: Delayed Detonation (Venom Splasher)

**Scope:** Mark a target, then detonate after a timer for AoE damage

**What exists:** Nothing similar. activeCasts handles cast times but not delayed detonations.

**What's new:**
- `venomSplasherMarks` Map tracking marked targets
- Each mark: `{ casterId, targetId, level, effectVal, detonateAt, attackerSnapshot }`
- In combat tick: check if any marks have reached `detonateAt`
- On detonation: calculate AoE damage, broadcast, death checks
- Cancel mark if target dies before detonation

**Effort:** MEDIUM (new timer system, unique mechanic)

---

### System 5: Enchant Poison Weapon Element Buff

**Scope:** Change weapon element to Poison temporarily, with on-hit poison proc

**What exists:** `getCombinedModifiers()` supports `weaponElement` override from buffs. Aspersio (Holy endow) may already use this pattern.

**What's new:**
- Apply `enchant_poison` buff with `weaponElement: 'poison'`
- Add on-hit poison proc check in auto-attack damage path
- Remove buff on weapon switch

**Integration:**
- Reuses existing weapon element endow buff pattern
- Poison status application uses existing `applyStatusEffect` system

**Effort:** LOW (mostly reuses existing infrastructure)

---

### System 6: Katar Weapon Validation

**Scope:** Check if player has a Katar equipped for katar-only skills

**What exists:** `isKatar(subType)` helper function. `player.equippedWeaponRight.subType` available.

**What's new:**
- Helper function: `hasKatarEquipped(player)` -> checks right-hand weapon is Katar type
- Used by: Sonic Blow, Grimtooth, Advanced Katar Mastery
- Already have `isKatar()` -- just need to integrate into skill handlers

**Effort:** TRIVIAL

---

### System 7: Red Gemstone Catalyst Consumption

**Scope:** Check and consume Red Gemstone for Venom Dust and Venom Splasher

**What exists:** `SKILL_CATALYSTS` constant maps skill IDs to required items. Used by Stone Curse, Safety Wall, Warp Portal.

**What's new:**
- Add entries to `SKILL_CATALYSTS`:
  - Venom Dust (1105): Red Gemstone
  - Venom Splasher (1110): Red Gemstone
- The catalyst check/consumption pipeline already exists

**Effort:** TRIVIAL (just add entries to existing constant)

---

### System 8: EDP Damage Modifier Integration

**Scope:** Modify damage calculation when EDP buff is active

**What exists:** Buff system tracks active buffs. `getCombinedModifiers()` returns buff effects.

**What's new:**
- New modifier in getCombinedModifiers: `edpMultiplier` (from EDP buff)
- In `calculatePhysicalDamage()` or at call sites:
  - For auto-attacks: multiply weapon ATK by `edpMultiplier`
  - For skills: check if skill is EDP-affected, halve modifier if so, then apply `edpMultiplier`
  - For EDP-immune skills: skip entirely
- Deadly Poison status effect application on each hit

**EDP-Affected Skills List:**
- Sonic Blow: YES (halved modifier)
- Soul Destroyer: NO
- Grimtooth: NO
- Meteor Assault: NO
- Venom Knife: NO
- Auto-attacks: YES (full multiplier, no halving)

**Effort:** MEDIUM (requires careful integration into damage path)

---

### System 9: Soul Destroyer Hybrid Damage

**Scope:** Unique damage calculation combining physical + magical components

**What exists:** `calculatePhysicalDamage()` and `calculateMagicalDamage()` as separate functions.

**What's new:**
- `calculateSoulDestroyerDamage(attacker, target, skillLevel)`:
  1. Physical: `(TotalATK - LeftHandATK) * SkillLv` -- subject to HIT/FLEE, DEF, elements
  2. INT: `INT * 5 * SkillLv + random(500, 1000)` -- always hits, ignores MDEF
  3. Total = Physical + INT
- Cannot simply call existing functions because:
  - Physical part excludes left-hand ATK
  - INT part is NOT MATK-based (does not use MATK formula)
  - INT part ignores MDEF entirely

**Effort:** MEDIUM (custom damage function)

---

### System 10: Create Deadly Poison Crafting

**Scope:** Material consumption + success/failure crafting with self-damage on failure

**What exists:** Item consumption for catalysts. Inventory checking.

**What's new:**
- Check 7 specific items in inventory
- Consume all 7 on attempt (success or failure)
- Roll success rate based on DEX + LUK
- On success: add Poison Bottle to inventory
- On failure: self-damage 20% MaxHP (min HP = 1)

**Effort:** MEDIUM (multi-item consumption + crafting)

---

## Existing Systems That Can Be Reused

| System | Used By | Reuse For |
|--------|---------|-----------|
| `executePhysicalSkillOnEnemy()` | Bash, Envenom, Double Strafe, etc. | Sonic Blow, Grimtooth (primary target), Poison React counter |
| `calculatePhysicalDamage()` | All physical skills | All Assassin physical skills |
| `applyBuff()` / `expireBuffs()` | Provoke, Blessing, etc. | Enchant Poison, Poison React, Cloaking, EDP |
| `getCombinedModifiers()` | All buff checks | Cloaking (isHidden), EP (weaponElement), EDP |
| `applyStatusEffect()` | Envenom, Frost Diver, etc. | Sonic Blow (stun), Venom Dust (poison), EP (poison proc), Meteor Assault (stun/blind/bleed) |
| `getPassiveSkillBonuses()` | 12 existing passives | Advanced Katar Mastery |
| `SKILL_CATALYSTS` | Stone Curse, Safety Wall | Venom Dust, Venom Splasher |
| `isKatar()` / `canDualWield()` | Dual wield system | Katar weapon validation |
| `getEffectiveStats()` | All stat calculations | Soul Destroyer INT component |
| Hiding toggle handler | Thief Hiding | Cloaking toggle (reuse pattern) |
| Hiding SP drain tick | Thief Hiding (if implemented) | Cloaking SP drain (different intervals) |
| `activeCasts` Map | Cast time system | Venom Splasher cast, Soul Destroyer cast, Meteor Assault cast |
| `broadcastToZone()` | All zone events | All skill broadcasts |
| `processEnemyDeathFromSkill()` | All offensive skills | All Assassin offensive skills |
| `interruptCast()` | Cast interruption | Venom Splasher, Soul Destroyer, Meteor Assault cast interruption |

---

## Implementation Priority

### Phase 1: Core Assassin Skills (Highest Impact)

These skills define the Assassin class identity and are the most commonly used.

| Priority | Skill | Handler Type | Effort | Dependencies |
|----------|-------|-------------|--------|--------------|
| 1 | Sonic Blow (1101) | Physical single-target | LOW | Katar check |
| 2 | Enchant Poison (1109) | Self-buff | LOW | Buff system |
| 3 | Cloaking (1103) | Toggle | LOW-MED | Hiding system |
| 4 | Skill definition fixes | Data corrections | LOW | None |

**Deliverables:**
- Sonic Blow handler with katar check, stun chance, Sonic Acceleration bonus
- Enchant Poison buff with weapon element override and on-hit poison proc
- Cloaking toggle with SP drain (reuses Hiding infrastructure)
- Fix all skill definition issues identified in audit table

### Phase 2: Stealth & Poison Skills

| Priority | Skill | Handler Type | Effort | Dependencies |
|----------|-------|-------------|--------|--------------|
| 5 | Grimtooth (1102) | Physical AoE from hiding | MEDIUM | Cloaking/Hiding |
| 6 | Throw Venom Knife (1111) | Ranged physical | LOW | Ammo consumption |
| 7 | Venom Dust (1105) | Ground effect | MEDIUM | Ground effect system |
| 8 | Poison React (1104) | Reactive buff | MEDIUM | Damage hook system |

**Deliverables:**
- Grimtooth with 3x3 AoE, hiding requirement, does-not-break-hiding
- Venom Knife with ammo consumption
- Venom Dust ground poison zone (introduces ground effect system)
- Poison React reactive counter (introduces damage hook system)

### Phase 3: Advanced Skills (Venom Splasher + Assassin Cross)

| Priority | Skill | Handler Type | Effort | Dependencies |
|----------|-------|-------------|--------|--------------|
| 9 | Venom Splasher (1110) | Delayed detonation AoE | HIGH | Detonation system |
| 10 | Advanced Katar Mastery | Passive | LOW | None |
| 11 | Create Deadly Poison | Crafting | MEDIUM | Multi-item consumption |
| 12 | EDP | Self-buff | MEDIUM | EDP damage modifier |
| 13 | Soul Destroyer | Hybrid damage | MEDIUM | Custom damage calc |
| 14 | Meteor Assault | Self-centered AoE | LOW-MED | Status effects |

**Deliverables:**
- Venom Splasher with delayed detonation system
- All 5 Assassin Cross skills defined and handled
- EDP damage modifier integration into damage pipeline
- Soul Destroyer hybrid damage calculation

---

## Assassin-Specific Constants & Data Tables

### Cloaking SP Drain Intervals (seconds per 1 SP drain)

```js
const CLOAKING_SP_DRAIN_INTERVALS = [0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0];
// Index = skillLevel - 1
```

### Cloaking Movement Speed (% of base speed)

```js
const CLOAKING_SPEED_ON_WALL  = [103, 106, 109, 112, 115, 118, 121, 124, 127, 130];
const CLOAKING_SPEED_OFF_WALL = [  0,   0,  79,  82,  85,  88,  91,  94,  97, 100];
// 0 = cannot move (wall required at Lv1-2)
```

### Poison React SP Cost (not a clean formula)

```js
const POISON_REACT_SP_COST = [25, 30, 35, 40, 45, 50, 55, 60, 45, 45];
```

### Poison React Envenom Counter Limits

```js
const POISON_REACT_ENVENOM_LIMIT = [1, 1, 2, 2, 3, 3, 4, 4, 5, 6];
// Formula: floor((level+1)/2) for Lv1-9, 6 for Lv10
```

### Venom Splasher Explosion Timer (ms)

```js
const VENOM_SPLASHER_TIMER = [5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500];
// Formula: (4.5 + 0.5 * level) * 1000
```

### Venom Splasher Cooldown (ms)

```js
const VENOM_SPLASHER_COOLDOWN = [7500, 8000, 8500, 9000, 9500, 10000, 10500, 11000, 11500, 12000];
// Formula: (7 + 0.5 * level) * 1000
```

### Soul Destroyer SP Cost

```js
const SOUL_DESTROYER_SP_COST = [20, 20, 20, 20, 20, 30, 30, 30, 30, 30];
```

### Soul Destroyer After-Cast Delay (ms)

```js
const SOUL_DESTROYER_ACD = [1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800];
// Formula: (0.8 + 0.2 * level) * 1000
```

### Meteor Assault SP Cost (Pre-Renewal)

```js
const METEOR_ASSAULT_SP_COST = [10, 12, 14, 16, 18, 20, 22, 24, 26, 28];
// Formula: 8 + 2 * level
```

### EDP Multiplier Table

```js
const EDP_MULTIPLIER = [2.0, 2.5, 3.0, 3.5, 4.0];
// Formula: 1.5 + 0.5 * level
```

### EDP Duration (ms)

```js
const EDP_DURATION = [40000, 45000, 50000, 55000, 60000];
// Formula: (35 + 5 * level) * 1000
```

### EDP-Immune Skills (skill names)

```js
const EDP_IMMUNE_SKILLS = new Set([
    'grimtooth',
    'throw_venom_knife',
    'meteor_assault',
    'soul_destroyer'
]);
```

### EDP Half-Modifier Skills

```js
const EDP_HALF_MODIFIER_SKILLS = new Set([
    'sonic_blow'
]);
```

### SKILL_CATALYSTS Additions

```js
// Add to existing SKILL_CATALYSTS:
1105: { itemId: 716, itemName: 'Red Gemstone', quantity: 1 },  // Venom Dust
1110: { itemId: 716, itemName: 'Red Gemstone', quantity: 1 },  // Venom Splasher
```

### Assassin Cross Skill ID Range

Recommended ID range for Assassin Cross skills: **2100-2119** (following existing 2nd class ranges).

| ID | Skill Name |
|----|-----------|
| 2100 | advanced_katar_mastery |
| 2101 | create_deadly_poison |
| 2102 | enchant_deadly_poison |
| 2103 | soul_destroyer |
| 2104 | meteor_assault |

---

## Integration Points

### 1. Combat Tick (index.js)
- **EDP modifier**: When attacker has EDP buff, multiply weapon ATK in auto-attack path
- **Poison React hook**: When player with poison_react buff takes damage, trigger counter
- **Venom Splasher tick**: Check detonation timers in combat tick loop

### 2. Buff Expiry Tick (index.js)
- **Cloaking SP drain**: Add Cloaking drain intervals alongside Hiding drain
- **Ground effect cleanup**: Remove expired Venom Dust zones

### 3. Position Broadcast (index.js)
- **Cloaking allows movement**: Do NOT reject position updates for cloaked players (unlike Hiding)
- **Grimtooth from hiding**: Position updates while hidden (for Cloaking, not Hiding)

### 4. Skill:use Handler (index.js)
- **Katar check**: Sonic Blow and Grimtooth require katar
- **Hidden check**: Grimtooth requires isHidden=true
- **Cloaking breaks on skill use**: Any skill except Cloaking toggle itself breaks cloak
- **Grimtooth exception**: Grimtooth does NOT break hiding/cloaking

### 5. calculatePhysicalDamage (ro_damage_formulas.js)
- **EDP multiplier**: New option `edpMultiplier` to scale weapon ATK
- **EDP skill halving**: New option `edpHalveSkillMod` for Sonic Blow
- Or handle EDP at the call site (multiply effectVal before passing)

### 6. getPassiveSkillBonuses (index.js)
- **Advanced Katar Mastery**: Add `katarDamagePercent` bonus for SinX with katar

### 7. Client VFX (SkillVFXData.cpp)
- Each new skill needs a VFX config entry
- Sonic Blow: `AoEImpact` or custom multi-hit template
- Grimtooth: `AoEImpact` (3x3 area)
- Enchant Poison: `SelfBuff` (poison aura)
- Cloaking: `SelfBuff` (fade effect + invisibility)
- Venom Dust: `GroundPersistent` (poison cloud)
- Venom Splasher: `TargetDebuff` (mark) + `AoEImpact` (explosion)
- Poison React: `SelfBuff` (reactive barrier)
- Soul Destroyer: `Projectile` (ranged dark/shadow bolt)
- Meteor Assault: `AoEImpact` (self-centered explosion)
- EDP: `SelfBuff` (dark poison aura)

### 8. Client Skill Tree (auto-discovery)
- Skills appear automatically via `skills:data` event based on `classId` and `treeRow`/`treeCol`
- Need treeRow/treeCol values for Assassin Cross skills

### 9. Hotbar Integration
- All active skills auto-work with hotbar via existing `skill:use` emission
- Quest skills may need special handling for quest completion checks

### 10. Weapon Switch Hook
- Enchant Poison buff removed on weapon switch
- EDP buff removed on weapon switch

---

## Sources

- [iRO Wiki Classic -- Assassin](https://irowiki.org/classic/Assassin)
- [iRO Wiki Classic -- Assassin Cross](https://irowiki.org/classic/Assassin_Cross)
- [iRO Wiki Classic -- Sonic Blow](https://irowiki.org/classic/Sonic_Blow)
- [iRO Wiki Classic -- Grimtooth](https://irowiki.org/classic/Grimtooth)
- [iRO Wiki Classic -- Cloaking](https://irowiki.org/classic/Cloaking)
- [iRO Wiki Classic -- Soul Destroyer](https://irowiki.org/classic/Soul_Destroyer)
- [iRO Wiki Classic -- Enchant Deadly Poison](https://irowiki.org/classic/Enchant_Deadly_Poison)
- [iRO Wiki -- Enchant Poison](https://irowiki.org/wiki/Enchant_Poison)
- [iRO Wiki -- Venom Dust](https://irowiki.org/wiki/Venom_Dust)
- [iRO Wiki -- Venom Splasher](https://irowiki.org/wiki/Venom_Splasher)
- [iRO Wiki -- Poison React](https://irowiki.org/wiki/Poison_React)
- [iRO Wiki -- Sonic Acceleration](https://irowiki.org/wiki/Sonic_Acceleration)
- [iRO Wiki -- Katar Mastery](https://irowiki.org/wiki/Katar_Mastery)
- [iRO Wiki -- Righthand Mastery](https://irowiki.org/wiki/Righthand_Mastery)
- [iRO Wiki -- Lefthand Mastery](https://irowiki.org/wiki/Lefthand_Mastery)
- [iRO Wiki -- Advanced Katar Mastery](https://irowiki.org/wiki/Advanced_Katar_Mastery)
- [iRO Wiki -- Meteor Assault](https://irowiki.org/wiki/Meteor_Assault)
- [iRO Wiki -- Soul Destroyer](https://irowiki.org/wiki/Soul_Destroyer)
- [iRO Wiki -- Enchant Deadly Poison](https://irowiki.org/wiki/Enchant_Deadly_Poison)
- [iRO Wiki -- Create Deadly Poison](https://irowiki.org/wiki/Create_Deadly_Poison)
- [iRO Wiki -- Venom Knife](https://irowiki.org/wiki/Venom_Knife)
- [iRO Wiki -- Cloaking](https://irowiki.org/wiki/Cloaking)
- [RateMyServer -- Assassin Skills (Pre-RE)](https://ratemyserver.net/index.php?page=skill_db&jid=12)
- [RateMyServer -- Assassin Cross Skills (Pre-RE)](https://ratemyserver.net/index.php?page=skill_db&jid=4013)
- [RateMyServer -- Sonic Blow](https://ratemyserver.net/index.php?page=skill_db&skid=136)
- [RateMyServer -- Grimtooth](https://ratemyserver.net/index.php?page=skill_db&skid=137)
- [RateMyServer -- Enchant Poison](https://ratemyserver.net/index.php?page=skill_db&skid=138)
- [RateMyServer -- Poison React](https://ratemyserver.net/index.php?page=skill_db&skid=139)
- [RateMyServer -- Venom Splasher](https://ratemyserver.net/index.php?page=skill_db&skid=141)
- [RateMyServer -- Soul Destroyer](https://ratemyserver.net/index.php?page=skill_db&skid=379)
- [RateMyServer -- Meteor Assault](https://ratemyserver.net/index.php?page=skill_db&skid=406)
- [RateMyServer -- EDP](https://ratemyserver.net/index.php?page=skill_db&skid=378)
- [Ragnarok Wiki Fandom -- Soul Breaker](https://ragnarok.fandom.com/wiki/Soul_Breaker)
- [Ragnarok Wiki Fandom -- Enchant Deadly Poison](https://ragnarok.fandom.com/wiki/Enchant_Deadly_Poison)
- Existing project files: `server/src/ro_skill_data_2nd.js`, `server/src/index.js`, `server/src/ro_buff_system.js`, `server/src/ro_damage_formulas.js`
- Existing project docs: `RagnaCloneDocs/03_Skills_Complete.md`, `docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md`
