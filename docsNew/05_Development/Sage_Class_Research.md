# Sage Class — Comprehensive Implementation Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Sage_Skills_Audit](Sage_Skills_Comprehensive_Audit.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE — Ready for implementation
**Scope:** All 22 Sage skills (IDs 1400-1421) + 2 Quest skills (1420-1421)
**Sources:** iRO Wiki (Classic + Renewal), RateMyServer (Pre-Re), rAthena pre-renewal DB, RagnaCloneDocs

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Complete Skill List](#2-complete-skill-list)
3. [Skill Tree (Prerequisites)](#3-skill-tree-prerequisites)
4. [Passive Skills (No Handler)](#4-passive-skills-no-handler)
5. [Endow Skills (Weapon Element Buff)](#5-endow-skills-weapon-element-buff)
6. [Ground Effect Skills (Volcano / Deluge / Violent Gale)](#6-ground-effect-skills-volcano--deluge--violent-gale)
7. [Land Protector](#7-land-protector)
8. [Anti-Mage Skills (Magic Rod / Spell Breaker / Dispell)](#8-anti-mage-skills-magic-rod--spell-breaker--dispell)
9. [Offensive Magic (Earth Spike / Heaven's Drive)](#9-offensive-magic-earth-spike--heavens-drive)
10. [Hindsight (Auto Spell)](#10-hindsight-auto-spell)
11. [Utility Skills (Cast Cancel / Sense / Abracadabra)](#11-utility-skills-cast-cancel--sense--abracadabra)
12. [Quest Skills (Create Converter / Elemental Change)](#12-quest-skills-create-converter--elemental-change)
13. [New Systems Required](#13-new-systems-required)
14. [Integration Points](#14-integration-points)
15. [Implementation Priority](#15-implementation-priority)
16. [Constants and Data Tables](#16-constants-and-data-tables)
17. [Current Codebase Status](#17-current-codebase-status)

---

## 1. Class Overview

| Property | Value |
|----------|-------|
| Base Class | Mage |
| Transcendent | Scholar (Professor) |
| Role | Support / Anti-Mage / Battle Caster |
| Weapons | Book, Rod/Staff |
| Skill Points | 49 (Job Level 1-50) |
| CLASS_PROGRESSION | `['novice', 'mage', 'sage']` |
| Inherits | All Novice + Mage skills (IDs 1-3, 200-213) |

**Unique Identity:** The Sage is a hybrid support/anti-mage class. Unlike the Wizard who focuses purely on offensive AoE magic, the Sage specializes in:
- **Weapon Endow** -- temporarily changing a party member's weapon element (Fire/Water/Wind/Earth)
- **Ground Effects** -- creating elemental zones (Volcano/Deluge/Violent Gale) that buff allies
- **Anti-Magic** -- absorbing/interrupting enemy spells (Magic Rod, Spell Breaker, Dispell)
- **Battle Sage** -- melee+magic hybrid via Hindsight (auto-cast bolts on physical attack)
- **Earth Magic** -- Earth Spike and Heaven's Drive as primary offensive spells

---

## 2. Complete Skill List

### Regular Skills (20 skills)

| ID | Internal Name | Display Name | Type | Target | Element | Max Lv | rA ID |
|----|--------------|--------------|------|--------|---------|--------|-------|
| 1400 | `advanced_book` | Advanced Book | Passive | None | Neutral | 10 | 274 |
| 1401 | `cast_cancel` | Cast Cancel | Active | Self | Neutral | 5 | 275 |
| 1402 | `hindsight` | Hindsight | Active | Self | Neutral | 10 | 279 |
| 1403 | `dispell` | Dispell | Active | Single | Neutral | 5 | 289 |
| 1404 | `magic_rod` | Magic Rod | Active | Self | Neutral | 5 | 276 |
| 1405 | `free_cast` | Free Cast | Passive | None | Neutral | 10 | 278 |
| 1406 | `spell_breaker` | Spell Breaker | Active | Single | Neutral | 5 | 277 |
| 1407 | `dragonology` | Dragonology | Passive | None | Neutral | 5 | 284 |
| 1408 | `endow_blaze` | Endow Blaze | Active | Single | Fire | 5 | 280 |
| 1409 | `endow_tsunami` | Endow Tsunami | Active | Single | Water | 5 | 281 |
| 1410 | `endow_tornado` | Endow Tornado | Active | Single | Wind | 5 | 282 |
| 1411 | `endow_quake` | Endow Quake | Active | Single | Earth | 5 | 283 |
| 1412 | `volcano` | Volcano | Active | Ground | Fire | 5 | 285 |
| 1413 | `deluge` | Deluge | Active | Ground | Water | 5 | 286 |
| 1414 | `violent_gale` | Violent Gale | Active | Ground | Wind | 5 | 287 |
| 1415 | `land_protector` | Land Protector | Active | Ground | Neutral | 5 | 288 |
| 1416 | `abracadabra` | Abracadabra | Active | Self | Neutral | 10 | 290 |
| 1417 | `earth_spike_sage` | Earth Spike | Active | Single | Earth | 5 | 90 |
| 1418 | `heavens_drive_sage` | Heaven's Drive | Active | Ground | Earth | 5 | 91 |
| 1419 | `sense_sage` | Sense | Active | Single | Neutral | 1 | 93 |

### Quest Skills (2 skills)

| ID | Internal Name | Display Name | Type | Target | Max Lv | rA ID |
|----|--------------|--------------|------|--------|--------|-------|
| 1420 | `create_elemental_converter` | Create Converter | Active | Self | 1 | 1007 |
| 1421 | `elemental_change` | Elemental Change | Active | Single | 1 | 1008/1017-1019 |

---

## 3. Skill Tree (Prerequisites)

### Mage Skill Prerequisites (Required Before Sage Skills)

The Sage inherits all Mage skills. Several Sage skills require specific Mage skills:

| Sage Skill | Requires Mage Skill |
|-----------|---------------------|
| Endow Blaze (1408) | Fire Bolt (201) Lv1 |
| Endow Tsunami (1409) | Cold Bolt (200) Lv1 |
| Endow Tornado (1410) | Lightning Bolt (202) Lv1 |
| Endow Quake (1411) | Stone Curse (206) Lv1 |
| Earth Spike (1417) | Stone Curse (206) Lv1 |

### Sage Internal Skill Tree

```
Advanced Book (1400) Lv2 --> Cast Cancel (1401) Lv1 --> Free Cast (1405)
Advanced Book (1400) Lv4 --> Magic Rod (1404) Lv1 --> Spell Breaker (1406)
Advanced Book (1400) Lv5 --> Endow Blaze (1408) Lv2 --> Volcano (1412) Lv3 --|
Advanced Book (1400) Lv5 --> Endow Tsunami (1409) Lv2 --> Deluge (1413) Lv3 --|--> Land Protector (1415)
Advanced Book (1400) Lv5 --> Endow Tornado (1410) Lv2 --> Violent Gale (1414) Lv3 --|
Advanced Book (1400) Lv9 --> Dragonology (1407)
Free Cast (1405) Lv4 --> Hindsight (1402)
Spell Breaker (1406) Lv3 --> Dispell (1403)
Stone Curse (206) Lv1 + Endow Quake (1411) Lv1 --> Earth Spike (1417) Lv1 --> Heaven's Drive (1418)
Hindsight (1402) Lv5 + Land Protector (1415) Lv1 + Dispell (1403) Lv1 --> Abracadabra (1416)
```

### Skill Tree Grid Layout (treeRow / treeCol)

```
Row 0: [Advanced Book]  [Cast Cancel]  [Magic Rod]   [Sense]
Row 1: [Free Cast]      [Spell Breaker] [Hindsight]  [Dragonology]
Row 2: [Endow Blaze]    [Endow Tsunami] [Endow Tornado] [Endow Quake]
Row 3: [Volcano]         [Deluge]       [Violent Gale] [Dispell]
Row 4: [Land Protector]  [Abracadabra]  [Earth Spike]  [Heaven's Drive]
Row 5: [Create Converter] [Elem Change]  (quest skills)
```

---

## 4. Passive Skills (No Handler)

### 4.1 Advanced Book (Study) — ID 1400

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Weapon Requirement | Book weapons |
| Effect | +3 ATK per level with Books, +0.5% ASPD per level |

**Per-Level Table:**

| Level | ATK Bonus | ASPD Bonus |
|-------|-----------|-----------|
| 1 | +3 | +0.5% |
| 2 | +6 | +1.0% |
| 3 | +9 | +1.5% |
| 4 | +12 | +2.0% |
| 5 | +15 | +2.5% |
| 6 | +18 | +3.0% |
| 7 | +21 | +3.5% |
| 8 | +24 | +4.0% |
| 9 | +27 | +4.5% |
| 10 | +30 | +5.0% |

**Implementation:** Add to `getPassiveSkillBonuses()`:
```js
// Advanced Book (1400): +3 ATK/level with Books, +0.5% ASPD/level
const abLv = learned[1400] || 0;
if (abLv > 0 && wType === 'book') {
    bonuses.bonusATK += abLv * 3;
    bonuses.bookAspdBonus = abLv * 0.5; // percentage
}
```

**New field required in bonuses object:** `bookAspdBonus` (float, percentage points to add to ASPD calculation).

### 4.2 Free Cast — ID 1405

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Prerequisite | Cast Cancel Lv1 |
| Effect | Allows movement while casting at reduced speed |

**Per-Level Table:**

| Level | Movement Speed While Casting | ASPD While Casting |
|-------|------------------------------|---------------------|
| 1 | 30% | 55% |
| 2 | 35% | 60% |
| 3 | 40% | 65% |
| 4 | 45% | 70% |
| 5 | 50% | 75% |
| 6 | 55% | 80% |
| 7 | 60% | 85% |
| 8 | 65% | 90% |
| 9 | 70% | 95% |
| 10 | 75% | 100% |

**Implementation:** This is a MAJOR new system. Currently, movement during casting triggers `interruptCast()` in the `player:position` handler. Free Cast needs to:
1. Check `player.learnedSkills[1405] > 0` before interrupting cast on movement
2. If Free Cast is learned, allow movement during cast (no interruption from position updates)
3. Movement speed during cast is reduced to `(25 + freeCastLv * 5)%` of normal speed
4. The cast STILL gets interrupted by taking damage (Free Cast does NOT prevent damage interruption)
5. Free Cast does NOT prevent flinch interruption -- only movement is allowed, not immunity

**Server-side changes needed:**
- `player:position` handler: skip `interruptCast()` when player has Free Cast
- Store `player.freeCastLevel` on join for quick checks
- Client-side: movement speed reduction during casting (server validates position rate)

### 4.3 Dragonology — ID 1407

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Prerequisite | Advanced Book Lv9 |
| Effect | Stat bonuses and combat bonuses vs Dragon race |

**Per-Level Table (Pre-Renewal):**

| Level | INT Bonus | Dragon ATK Bonus | Dragon Resist |
|-------|-----------|------------------|---------------|
| 1 | +1 | +4% | +4% |
| 2 | +1 | +8% | +8% |
| 3 | +1 | +12% | +12% |
| 4 | +2 | +16% | +16% |
| 5 | +3 | +20% | +20% |

**Implementation:** Add to `getPassiveSkillBonuses()`:
```js
// Dragonology (1407): INT bonus + Dragon ATK/resist
const dragLv = learned[1407] || 0;
if (dragLv > 0) {
    bonuses.bonusINT = (bonuses.bonusINT || 0) + [1, 1, 1, 2, 3][dragLv - 1];
    const pct = dragLv * 4;
    bonuses.raceATK.dragon = (bonuses.raceATK.dragon || 0) + pct;
    bonuses.raceResist = bonuses.raceResist || {};
    bonuses.raceResist.dragon = (bonuses.raceResist.dragon || 0) + pct;
}
```

**New fields required:**
- `bonusINT` -- new field in bonuses object (integer, flat INT bonus)
- `raceResist.dragon` -- damage reduction % when attacked by dragons

---

## 5. Endow Skills (Weapon Element Buff)

All four endow skills share the same mechanics with different elements and catalysts.

### 5.1 Core Mechanics (Shared)

| Property | Value |
|----------|-------|
| SP Cost | 40 (all levels) |
| Cast Time (pre-renewal) | 3000ms (all levels) |
| Target Type | Single ally (party/guild member, or self) |
| Range | 9 cells (450 UE units) |

### 5.2 Per-Level Table (Pre-Renewal)

| Level | Success Rate | Duration | effectValue in current def |
|-------|-------------|----------|---------------------------|
| 1 | 70% | 20 min (1,200,000ms) | 70 |
| 2 | 80% | 22.5 min (1,320,000ms) | 76 |
| 3 | 90% | 25 min (1,500,000ms) | 82 |
| 4 | 100% | 27.5 min (1,620,000ms) | 88 |
| 5 | 100% | 30 min (1,800,000ms) | 94 |

**Note on current skill data:** The current `effectValue: 70+i*6` encodes success rate (70, 76, 82, 88, 94). The actual success rates should be 70/80/90/100/100. The duration formula `1200000+i*120000` gives 1.2M/1.32M/1.44M/1.56M/1.68M -- this needs audit.

**Corrected duration formula (pre-renewal):** All levels = 20 minutes (1,200,000ms), except Lv5 = 30 minutes (1,800,000ms). Alternative: some sources say uniform 20 min for all levels. The rAthena canonical data shows duration scaling.

### 5.3 Endow Blaze (Fire) — ID 1408

| Property | Value |
|----------|-------|
| Element Endowed | Fire |
| Catalyst | 1 Red Blood (Item ID 990) |
| Mage Prereq | Fire Bolt (201) Lv1 |
| Sage Prereq | Advanced Book (1400) Lv5 |

### 5.4 Endow Tsunami (Water) — ID 1409

| Property | Value |
|----------|-------|
| Element Endowed | Water |
| Catalyst | 1 Crystal Blue (Item ID 991) |
| Mage Prereq | Cold Bolt (200) Lv1 |
| Sage Prereq | Advanced Book (1400) Lv5 |

### 5.5 Endow Tornado (Wind) — ID 1410

| Property | Value |
|----------|-------|
| Element Endowed | Wind |
| Catalyst | 1 Wind of Verdure (Item ID 992) |
| Mage Prereq | Lightning Bolt (202) Lv1 |
| Sage Prereq | Advanced Book (1400) Lv5 |

### 5.6 Endow Quake (Earth) — ID 1411

| Property | Value |
|----------|-------|
| Element Endowed | Earth |
| Catalyst | 1 Green Live (Item ID 993) |
| Mage Prereq | Stone Curse (206) Lv1 |
| Sage Prereq | Advanced Book (1400) Lv5 |

### 5.7 Endow Failure Mechanic (Pre-Renewal Only)

In pre-renewal RO, endow skills at levels below max have a chance of failure:
- On **failure**: the target's weapon is **broken** (weapon ATK becomes 0 until repaired)
- This is a harsh penalty that makes Lv4-5 essential
- **Implementation recommendation:** For Sabri_MMO, implement the success rate but **skip the weapon-breaking penalty** (too punishing for an MMO with no repair NPC yet). On failure, simply consume the catalyst and SP but do not apply the endow.

### 5.8 Endow Handler Template

```js
if (skill.name === 'endow_blaze' || skill.name === 'endow_tsunami' ||
    skill.name === 'endow_tornado' || skill.name === 'endow_quake') {

    // Target must be a player (self or party member)
    if (!targetId) { socket.emit('skill:error', { message: 'No target' }); return; }

    const target = connectedPlayers.get(targetId);
    if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }

    // Range check
    // ...

    // Deduct SP
    player.mana = Math.max(0, player.mana - spCost);

    // Catalyst consumed in executePhysicalSkillOnEnemy (SKILL_CATALYSTS)
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Success rate check
    const successRate = [70, 80, 90, 100, 100][learnedLevel - 1];
    if (Math.random() * 100 >= successRate) {
        socket.emit('skill:error', { message: 'Endow failed!' });
        socket.emit('skill:used', { skillId, skillName: skill.name, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
        return;
    }

    // Determine element from skill name
    const endowElement = {
        'endow_blaze': 'fire', 'endow_tsunami': 'water',
        'endow_tornado': 'wind', 'endow_quake': 'earth'
    }[skill.name];

    // Apply weapon endow buff
    applyBuff(target, {
        skillId, name: 'endow_' + endowElement,
        casterId: characterId, casterName: player.characterName,
        weaponElement: endowElement,
        duration: levelData.duration
    });

    // Set target.weaponElement (overrides card/equipment element)
    target.weaponElement = endowElement;

    broadcastToZone(zone, 'skill:buff_applied', { ... });
    socket.emit('skill:used', { ... });
    return;
}
```

### 5.9 Endow Catalyst Entries for SKILL_CATALYSTS

```js
'endow_blaze':    [{ itemId: 990, quantity: 1 }],  // Red Blood
'endow_tsunami':  [{ itemId: 991, quantity: 1 }],  // Crystal Blue
'endow_tornado':  [{ itemId: 992, quantity: 1 }],  // Wind of Verdure
'endow_quake':    [{ itemId: 993, quantity: 1 }],  // Green Live
```

### 5.10 Endow Buff Behavior

- **Mutual exclusion**: Only one weapon endow can be active at a time. Applying a new endow removes the old one.
- **Overwrite**: If target already has an endow buff, the new one replaces it.
- **Expiry**: On buff expiry, revert `target.weaponElement` to the value from equipped cards/equipment (or 'neutral' if none).
- **Dispell interaction**: Endow buffs ARE removed by Dispell.
- **Zone transition**: Endow buffs survive zone transitions (persisted in buff system).

---

## 6. Ground Effect Skills (Volcano / Deluge / Violent Gale)

### 6.1 Shared Properties

| Property | Value |
|----------|-------|
| Cast Time (pre-renewal) | 5000ms (all levels) |
| Catalyst | 1 Yellow Gemstone (Item ID 715) per initial cast |
| Target | Ground |
| Range | 2 cells (100 UE units) -- NOTE: short range! |
| AoE Size | 7x7 cells (350 UE units radius) |
| Cannot overlap | with any other ground effect skill |

### 6.2 Volcano — ID 1412

| Property | Value |
|----------|-------|
| Element | Fire |
| Prerequisite | Endow Blaze Lv2 |

| Level | SP Cost | Fire Damage Boost | ATK/MATK Bonus | Duration |
|-------|---------|-------------------|----------------|----------|
| 1 | 48 | +10% | +10 | 1 min |
| 2 | 46 | +14% | +15 | 2 min |
| 3 | 44 | +17% | +20 | 3 min |
| 4 | 42 | +19% | +25 | 4 min |
| 5 | 40 | +20% | +30 | 5 min |

**Special:** Prevents Ice Wall on affected area.

### 6.3 Deluge — ID 1413

| Property | Value |
|----------|-------|
| Element | Water |
| Prerequisite | Endow Tsunami Lv2 |

| Level | SP Cost | Water Damage Boost | MaxHP Bonus | Duration |
|-------|---------|-------------------|-------------|----------|
| 1 | 48 | +10% | +5% | 1 min |
| 2 | 46 | +14% | +9% | 2 min |
| 3 | 44 | +17% | +12% | 3 min |
| 4 | 42 | +19% | +14% | 4 min |
| 5 | 40 | +20% | +15% | 5 min |

**Special:** Functions as shallow water for Aqua Benedicta and Water Ball.

### 6.4 Violent Gale — ID 1414

| Property | Value |
|----------|-------|
| Element | Wind |
| Prerequisite | Endow Tornado Lv2 |

| Level | SP Cost | Wind Damage Boost | FLEE Bonus | Duration |
|-------|---------|-------------------|-----------|----------|
| 1 | 48 | +10% | +3 | 1 min |
| 2 | 46 | +14% | +6 | 2 min |
| 3 | 44 | +17% | +9 | 3 min |
| 4 | 42 | +19% | +12 | 4 min |
| 5 | 40 | +20% | +15 | 5 min |

**Special:** Extends Fire Wall duration by +50%.

### 6.5 Ground Effect Implementation Requirements

This is a MAJOR new system. The project currently has **no persistent ground effect infrastructure**. Ground effects differ from buffs in that they are:
- Tied to a **location**, not to a player
- **Visible to all players** in the zone
- **Apply effects to anyone standing inside** (checked per tick or on entry)
- Can be **removed by other ground skills** (e.g., Land Protector removes all ground effects)
- **Don't follow the caster** -- they stay where cast

**New system: `activeGroundEffects` Map**

```js
// Server-side: Map<effectId, GroundEffect>
const activeGroundEffects = new Map();
let nextGroundEffectId = 1;

// GroundEffect structure:
{
    id: Number,              // Unique effect ID
    skillId: Number,         // Which skill created it
    skillName: String,       // 'volcano', 'deluge', 'violent_gale', 'land_protector'
    casterId: Number,        // Who cast it
    zone: String,            // Which zone
    centerX: Number,         // Ground position X
    centerY: Number,         // Ground position Y
    centerZ: Number,         // Ground position Z
    radius: Number,          // Radius in UE units (350 for 7x7, 450 for 9x9, 550 for 11x11)
    element: String,         // 'fire', 'water', 'wind', 'neutral'
    effects: Object,         // Skill-specific effect data
    createdAt: Number,       // Date.now()
    expiresAt: Number,       // createdAt + duration
    level: Number            // Skill level
}
```

**Ground Effect Tick (1000ms interval):**
1. Check all `activeGroundEffects` for expiry, remove expired ones
2. For each active ground effect, find all players/enemies standing inside its radius
3. Apply effects (ATK bonus, FLEE bonus, MaxHP bonus, element damage boost)
4. Broadcast `ground:effect_created` / `ground:effect_expired` events

**Socket Events:**

| Event | Direction | Payload |
|-------|-----------|---------|
| `ground:effect_created` | Server -> Zone | `{ effectId, skillName, centerX/Y/Z, radius, element, level, duration }` |
| `ground:effect_expired` | Server -> Zone | `{ effectId }` |

**Catalyst rule:** First cast of Volcano/Deluge/Violent Gale consumes 1 Yellow Gemstone. Recasting the same skill type while the buff is still active does NOT consume a gemstone (the old one is replaced).

### 6.6 Skill Data Corrections Needed

The current skill definitions use `SP: 40+i*2` which gives 40, 42, 44, 46, 48. The canonical values are **reversed**: 48, 46, 44, 42, 40 (SP cost decreases with level). Fix:

```js
spCost: 48 - i * 2  // Lv1=48, Lv2=46, Lv3=44, Lv4=42, Lv5=40
```

Also, the range is wrong: current `range: 450` but canonical is `range: 100` (2 cells). Fix:

```js
range: 100  // 2 cells
```

### 6.7 Ground Catalyst Entries for SKILL_CATALYSTS

```js
'volcano':       [{ itemId: 715, quantity: 1 }],  // Yellow Gemstone
'deluge':        [{ itemId: 715, quantity: 1 }],  // Yellow Gemstone
'violent_gale':  [{ itemId: 715, quantity: 1 }],  // Yellow Gemstone
```

---

## 7. Land Protector

### 7.1 Mechanics

| Property | Value |
|----------|-------|
| ID | 1415 |
| Max Level | 5 |
| Cast Time (pre-renewal) | 5000ms (all levels) |
| Target | Ground |
| Range | 3 cells (150 UE units) |
| Catalyst | 1 Blue Gemstone (717) + 1 Yellow Gemstone (715) |
| Prerequisites | Volcano Lv3, Deluge Lv3, Violent Gale Lv3 |

### 7.2 Per-Level Table

| Level | SP Cost | AoE Size | Duration |
|-------|---------|----------|----------|
| 1 | 66 | 7x7 (350) | 120s (2 min) |
| 2 | 62 | 7x7 (350) | 165s (2.75 min) |
| 3 | 58 | 9x9 (450) | 210s (3.5 min) |
| 4 | 54 | 9x9 (450) | 255s (4.25 min) |
| 5 | 50 | 11x11 (550) | 300s (5 min) |

**SP Formula:** `70 - skillLevel * 4` (66, 62, 58, 54, 50)
**Duration Formula:** `75 + skillLevel * 45` seconds (120, 165, 210, 255, 300)
**AoE Formula:** Lv1-2 = 7x7, Lv3-4 = 9x9, Lv5 = 11x11

### 7.3 Blocked Skills

Land Protector **nullifies and removes** the following ground-targeted skills when cast on its area:
- Safety Wall
- Pneuma
- Warp Portal
- Sanctuary
- Magnus Exorcismus
- Volcano, Deluge, Violent Gale
- Fire Wall
- Fire Pillar
- Thunderstorm (AoE)
- Storm Gust
- Lord of Vermilion
- Meteor Storm
- Quagmire
- Ice Wall

### 7.4 Skills NOT Blocked by Land Protector

- All Hunter trap skills (Land Mine, Ankle Snare, Claymore Trap, etc.)
- Grand Cross (Crusader)
- Non-ground-targeted spells (bolts, single-target magic)

### 7.5 Implementation

Land Protector creates a ground effect that:
1. On creation: removes all existing ground effects in its area
2. While active: prevents new ground effects from being placed in its area
3. Does NOT buff/debuff players -- it purely blocks other ground skills

**Catalyst Entry:**
```js
'land_protector':    [{ itemId: 717, quantity: 1 }, { itemId: 715, quantity: 1 }],
// Already exists in SKILL_CATALYSTS
```

### 7.6 Skill Data Corrections

Current range is `450`, should be `150` (3 cells). SP cost formula `50+i*4` gives 50, 54, 58, 62, 66 -- needs to be **reversed**: `66-i*4` = 66, 62, 58, 54, 50.

---

## 8. Anti-Mage Skills (Magic Rod / Spell Breaker / Dispell)

### 8.1 Magic Rod — ID 1404

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 2 (all levels) |
| Cast Time | 0ms (instant activation) |
| After-Cast Delay | 500ms |
| Target | Self |
| Prerequisite | Advanced Book Lv4 |

**Effect:** Creates a temporary shield that absorbs the next incoming single-target magic spell, nullifying its damage and restoring SP equal to a percentage of the absorbed spell's SP cost.

| Level | Active Duration | SP Absorbed |
|-------|----------------|-------------|
| 1 | 400ms | 20% of spell's SP cost |
| 2 | 600ms | 40% |
| 3 | 800ms | 60% |
| 4 | 1000ms | 80% |
| 5 | 1200ms | 100% |

**Duration Formula:** `200 + level * 200`ms
**SP Absorption Formula:** `level * 20`%

**Mechanics:**
- Only absorbs **single-target magic** (bolts, Soul Strike, Frost Diver, Jupitel Thunder)
- Does NOT absorb ground AoE magic (Storm Gust, Lord of Vermilion, Meteor Storm, Thunderstorm)
- Can absorb **multiple spells** during its duration (each one restores SP)
- If a Spell Breaker is used against a Magic Rod user, the Magic Rod absorbs it and gains 20% of the Spell Breaker user's Max SP

**Implementation:** This is a NEW SYSTEM -- "magic absorption buff". When Magic Rod is active:
1. Apply a buff `magic_rod_active` with the duration and SP absorption %
2. In all incoming single-target magic damage paths, check if target has `magic_rod_active` buff
3. If so: cancel the damage, restore SP to the target, remove one "charge" from the buff
4. Broadcast `skill:buff_applied` with visual indicator

### 8.2 Spell Breaker — ID 1406

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 10 (all levels) |
| Cast Time (pre-renewal) | 700ms |
| Target | Single Enemy |
| Range | 9 cells (450 UE units) |
| Prerequisite | Magic Rod Lv1 |

**Effect:** Interrupts a target's ongoing spell casting and absorbs SP.

| Level | SP Absorbed | Damage (Lv5 only) |
|-------|-------------|-------------------|
| 1 | 0% | None |
| 2 | 25% of target spell's SP cost | None |
| 3 | 50% | None |
| 4 | 75% | None |
| 5 | 100% + 2% of target MaxHP damage | +1% HP absorbed |

**Mechanics:**
- Target must be **currently casting** (checked via `activeCasts` Map)
- Interrupts the cast (removes from `activeCasts`)
- SP absorption: caster gains `(spAbsorb% / 100) * targetSpellSPCost` SP
- Lv5 special: also deals 2% of target's MaxHP as damage and heals caster for 1% of target MaxHP
- **vs Boss/Guardian monsters:** only 10% success rate for interruption, but Lv5 damage still applies
- **Ignores Phen Card** cast protection -- always interrupts if successful

**Implementation:**
```js
if (skill.name === 'spell_breaker') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target' }); return; }

    // Target can be enemy or player
    const targetCast = activeCasts.get(targetId);

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    if (targetCast) {
        // Boss check: 10% success rate
        const target = isEnemy ? enemies.get(targetId) : connectedPlayers.get(targetId);
        const isBoss = isEnemy && target && target.modeFlags?.boss;
        if (isBoss && Math.random() > 0.10) {
            // Boss resisted interruption, but Lv5 damage still applies below
        } else {
            // Interrupt the cast
            activeCasts.delete(targetId);
            io.to(zone).emit('skill:cast_interrupted_broadcast', { casterId: targetId, skillId: targetCast.skillId });

            // SP absorption
            const targetSpellSPCost = targetCast.spCost || 0;
            const absorbPct = [0, 25, 50, 75, 100][learnedLevel - 1];
            const spGained = Math.floor(targetSpellSPCost * absorbPct / 100);
            player.mana = Math.min(player.maxMana, player.mana + spGained);
        }
    }

    // Lv5 special: 2% MaxHP damage + 1% HP drain
    if (learnedLevel >= 5 && target) {
        const hpDamage = Math.floor(target.maxHealth * 0.02);
        target.health = Math.max(0, target.health - hpDamage);
        const hpDrain = Math.floor(target.maxHealth * 0.01);
        player.health = Math.min(player.maxHealth, player.health + hpDrain);
        // Broadcast damage event
    }

    socket.emit('skill:used', { ... });
    return;
}
```

### 8.3 Dispell — ID 1403

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 1 (all levels) |
| Cast Time (pre-renewal) | 2000ms |
| After-Cast Delay | 2000ms |
| Target | Single target (enemy or player) |
| Range | 9 cells (450 UE units) |
| Catalyst | 1 Yellow Gemstone (715) |
| Prerequisite | Spell Breaker Lv3 |

**Per-Level Table:**

| Level | Success Rate |
|-------|-------------|
| 1 | 60% |
| 2 | 70% |
| 3 | 80% |
| 4 | 90% |
| 5 | 100% |

**Effect:** Removes all active buffs from the target.

**Buffs that CAN be removed by Dispell:**
- All Endow skills (Blaze/Tsunami/Tornado/Quake)
- Blessing, Increase AGI, Angelus
- Improve Concentration
- Endure
- Two-Hand Quicken, Spear Quicken
- Provoke (considered beneficial to remove enemy's ATK buff)
- Energy Coat
- Loud Exclamation
- Volcano/Deluge/Violent Gale stat bonuses
- Most standard buffs from `activeBuffs`

**Buffs that CANNOT be removed by Dispell:**
- Hindsight (Auto Spell) -- specifically immune
- Wedding/Cart/Riding status
- Soul Linker Spirit buffs (future)
- Status effects (stun, freeze, poison, etc.) -- these are status effects, not buffs

**Implementation:**
```js
if (skill.name === 'dispell') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target' }); return; }

    const target = isEnemy ? enemies.get(targetId) : connectedPlayers.get(targetId);
    if (!target) { socket.emit('skill:error', { message: 'Target not found' }); return; }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    // Success rate check
    const successRate = 60 + (learnedLevel - 1) * 10;
    if (Math.random() * 100 >= successRate) {
        socket.emit('skill:error', { message: 'Dispell failed!' });
        socket.emit('skill:used', { ... });
        return;
    }

    // Remove all dispellable buffs
    const UNDISPELLABLE = new Set(['hindsight', 'wedding', 'riding', 'cart']);
    if (target.activeBuffs) {
        const removed = [];
        target.activeBuffs = target.activeBuffs.filter(buff => {
            if (UNDISPELLABLE.has(buff.name)) return true; // keep
            removed.push(buff.name);
            return false; // remove
        });
        // Broadcast each removed buff
        for (const buffName of removed) {
            broadcastToZone(zone, 'skill:buff_removed', {
                targetId, isEnemy, buffName, reason: 'dispelled'
            });
        }
        // Revert weapon element if endow was removed
        if (removed.some(n => n.startsWith('endow_'))) {
            target.weaponElement = target.baseWeaponElement || 'neutral';
        }
    }

    broadcastToZone(zone, 'skill:effect_damage', {
        attackerId: characterId, targetId, skillId,
        damage: 0, hitType: 'skill', healAmount: 0,
        skillName: 'Dispell', ...
    });
    socket.emit('skill:used', { ... });
    return;
}
```

---

## 9. Offensive Magic (Earth Spike / Heaven's Drive)

### 9.1 Earth Spike — ID 1417

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Element | Earth |
| Target | Single enemy |
| Range | 9 cells (450 UE units) |
| Damage | 200% MATK per hit (multi-hit, N = skill level) |
| Note | This is a **Sage version** distinct from Wizard Earth Spike (ID 804). Same mechanics. |

**Per-Level Table:**

| Level | SP Cost | Cast Time | After-Cast Delay | Hits | Total Damage |
|-------|---------|-----------|-----------------|------|-------------|
| 1 | 12 | 700ms | 1000ms | 1 | 200% MATK |
| 2 | 14 | 1400ms | 1200ms | 2 | 400% MATK |
| 3 | 16 | 2100ms | 1400ms | 3 | 600% MATK |
| 4 | 18 | 2800ms | 1600ms | 4 | 800% MATK |
| 5 | 20 | 3500ms | 1800ms | 5 | 1000% MATK |

**Cast Time Formula:** `700 * level` ms
**SP Formula:** `10 + level * 2` (12, 14, 16, 18, 20)
**After-Cast Delay Formula:** `800 + level * 200` ms (1000, 1200, 1400, 1600, 1800)

**Sage Prerequisites:** Stone Curse (206) Lv1 + Endow Quake (1411) Lv1
(Different from Wizard prerequisite which is Stone Curse Lv1 only)

**Special Mechanics:**
- Unlike bolt skills, Earth Spike damage is bundled (all hits connect at once despite multi-hit animation)
- Can hit **hidden enemies** if cast while the enemy was still visible
- NOT a bolt skill -- incompatible with Double Bolt and Spell Fist (renewal systems)
- Each hit = 200% MATK (not 100% like bolts)

**Implementation:** Same as Wizard Earth Spike handler. The server handler should work for both `earth_spike` (804, wizard) and `earth_spike_sage` (1417, sage) since they share identical damage mechanics.

### 9.2 Heaven's Drive — ID 1418

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Element | Earth |
| Target | Ground (5x5 AoE) |
| Range | 9 cells (450 UE units) |
| AoE | 5x5 cells (250 UE units radius) |
| Damage | 125% MATK per hit (N = skill level) |

**Per-Level Table:**

| Level | SP Cost | Cast Time | After-Cast Delay | Hits | Total Damage |
|-------|---------|-----------|-----------------|------|-------------|
| 1 | 28 | 1000ms | 500ms | 1 | 125% MATK |
| 2 | 32 | 2000ms | 500ms | 2 | 250% MATK |
| 3 | 36 | 3000ms | 500ms | 3 | 375% MATK |
| 4 | 40 | 4000ms | 500ms | 4 | 500% MATK |
| 5 | 44 | 5000ms | 500ms | 5 | 625% MATK |

**Cast Time Formula:** `1000 * level` ms
**SP Formula:** `24 + level * 4` (28, 32, 36, 40, 44)
**After-Cast Delay:** 500ms (flat, all levels)

**Sage Prerequisites:** Earth Spike (1417) Lv1
(Compared to Wizard prerequisite: Earth Spike Lv3)

**Special Mechanics:**
- Can hit **hidden enemies** (like Earth Spike)
- Ground-targeted (does not require enemy to be targeted)
- All hits bundled despite multi-hit animation

**Skill Data Corrections Needed:**
Current SP formula `26+i*2` gives 26, 28, 30, 32, 34. Pre-renewal RateMyServer data shows different values depending on whether this is the pre-renewal or renewal version. The rAthena pre-renewal data for Heaven's Drive uses:
- SP: 28, 32, 36, 40, 44 (formula: `24 + level * 4`)

This needs audit -- the current definition may use the wrong formula.

**Note on current `effectValue`:** The current definition uses `effectValue: i+1` (= number of hits, same as Earth Spike). The damage formula is 125% MATK per hit. The handler should multiply MATK by 1.25 per hit, not use effectValue directly as ATK%.

---

## 10. Hindsight (Auto Spell)

### 10.1 Core Mechanics

| Property | Value |
|----------|-------|
| ID | 1402 |
| Max Level | 10 |
| SP Cost | 35 (all levels) |
| Cast Time | 2000ms (current def) / 3000ms (pre-renewal canonical) |
| Target | Self |
| Prerequisite | Free Cast Lv4 |
| Type | Buff (applies "hindsight" buff to self) |

### 10.2 Pre-Renewal Autocast Tables

**Autocast Chance Per Level (Pre-Renewal / RateMyServer):**

| Level | Autocast Chance | Duration | Spells Unlocked |
|-------|----------------|----------|-----------------|
| 1 | 7% | 120s (2 min) | Napalm Beat (up to Lv3) |
| 2 | 9% | 150s (2.5 min) | + Bolts (up to Lv1) |
| 3 | 11% | 180s (3 min) | + Bolts (up to Lv2) |
| 4 | 13% | 210s (3.5 min) | + Bolts (up to Lv3) |
| 5 | 15% | 240s (4 min) | + Soul Strike (up to Lv1) |
| 6 | 17% | 270s (4.5 min) | + Soul Strike (up to Lv2) |
| 7 | 20% | 300s (5 min) | + Soul Strike (up to Lv3) |
| 8 | 22% | 330s (5.5 min) | + Fire Ball (up to Lv1) |
| 9 | 23% | 360s (6 min) | + Fire Ball (up to Lv2) |
| 10 | 25% | 390s (6.5 min) | + Frost Diver (Lv1) |

**Duration Formula:** `90 + level * 30` seconds (120, 150, 180, ..., 390s)
**Autocast Chance Formula (pre-renewal):** NOT linear. Per RateMyServer: 7, 9, 11, 13, 15, 17, 20, 22, 23, 25

### 10.3 Autocast Bolt Level Distribution (Pre-Renewal)

When a bolt spell is triggered, the **level of the bolt** is randomly determined:
- **50% chance** = Level 1
- **35% chance** = Level 2
- **15% chance** = Level 3

The bolt level is capped by the player's learned level of that bolt. If the player has Fire Bolt Lv2, the autocast can only be Lv1 (50%) or Lv2 (50%, redistributed from the Lv2+3 pools).

### 10.4 Autocast SP Rules

- Autocasted spells consume **2/3 (66.7%)** of their normal SP cost
- If the player lacks sufficient SP, the autocast **does not trigger** (no partial casts)

### 10.5 Autocast Timing Rules

- Autocasted spells have **no cast time** (instant)
- Autocasted spells **do have after-cast delay** (but the delay does NOT prevent further physical attacks from triggering more autocasts)
- The autocast check happens **on every physical attack hit** (melee only)
- The attack does NOT need to hit the target to trigger the autocast check

### 10.6 Spell Selection Logic (Pre-Renewal)

When the autocast procs, the game randomly selects from the **available spells** at the current Hindsight level, weighted equally. The selected spell must also be **learned by the player** -- unlearned spells cannot be autocast.

Available spell pool at each Hindsight level:

| Hindsight Lv | Available Spells (if learned) |
|-------------|------------------------------|
| 1-3 | Cold Bolt, Fire Bolt, Lightning Bolt |
| 4-6 | + Soul Strike, Fire Ball |
| 7-9 | + Frost Diver, Earth Spike |
| 10 | + Thunderstorm, Heaven's Drive |

### 10.7 Hindsight Implementation

This is a MAJOR new system requiring changes to the **auto-attack combat tick**.

**Buff application:**
```js
if (skill.name === 'hindsight') {
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const chance = [7, 9, 11, 13, 15, 17, 20, 22, 23, 25][learnedLevel - 1];
    const duration = (90 + learnedLevel * 30) * 1000;

    applyBuff(player, {
        skillId, name: 'hindsight',
        casterId: characterId, casterName: player.characterName,
        autocastChance: chance,
        hindsightLevel: learnedLevel,
        duration
    });

    socket.emit('skill:used', { ... });
    return;
}
```

**Auto-attack tick modification:**
```js
// After each successful auto-attack hit (in processEnemyAutoAttack / processPlayerAutoAttack):
const hindsightBuff = attacker.activeBuffs?.find(b => b.name === 'hindsight');
if (hindsightBuff) {
    const roll = Math.random() * 100;
    if (roll < hindsightBuff.autocastChance) {
        // Determine available spells based on hindsightLevel
        const availableSpells = getHindsightSpellPool(hindsightBuff.hindsightLevel, attacker.learnedSkills);
        if (availableSpells.length > 0) {
            const spell = availableSpells[Math.floor(Math.random() * availableSpells.length)];
            const boltLevel = rollBoltLevel(attacker.learnedSkills[spell.skillId] || 0);
            const spCost = Math.floor(spell.baseSP * 2 / 3);
            if (attacker.mana >= spCost) {
                attacker.mana -= spCost;
                // Execute the spell instantly (no cast time, has ACD)
                executeAutocastSpell(attacker, characterId, spell, boltLevel, target, zone);
            }
        }
    }
}
```

**Bolt level roll function:**
```js
function rollBoltLevel(maxLearned) {
    if (maxLearned <= 0) return 0;
    if (maxLearned === 1) return 1;
    if (maxLearned === 2) {
        return Math.random() < 0.588 ? 1 : 2; // Redistributed: 50/(50+35) = 58.8% Lv1, 41.2% Lv2
    }
    // maxLearned >= 3
    const roll = Math.random() * 100;
    if (roll < 50) return 1;
    if (roll < 85) return 2;
    return 3;
}
```

### 10.8 Hindsight Undispellable

Hindsight is specifically immune to Dispell. The `UNDISPELLABLE` set must include `'hindsight'`.

---

## 11. Utility Skills (Cast Cancel / Sense / Abracadabra)

### 11.1 Cast Cancel — ID 1401

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 2 (all levels) |
| Cast Time | 0ms (instant) |
| Target | Self |
| Prerequisite | Advanced Book Lv2 |

**Effect:** Cancel the player's own ongoing spell cast, recovering a percentage of the SP that would have been consumed.

| Level | SP Retained |
|-------|------------|
| 1 | 10% |
| 2 | 30% |
| 3 | 50% |
| 4 | 70% |
| 5 | 90% |

**Implementation:**
```js
if (skill.name === 'cast_cancel') {
    const activeCast = activeCasts.get(characterId);
    if (!activeCast) {
        socket.emit('skill:error', { message: 'Not currently casting' });
        return;
    }

    // Cancel the cast
    activeCasts.delete(characterId);
    broadcastToZone(zone, 'skill:cast_interrupted_broadcast', { casterId: characterId, skillId: activeCast.skillId });

    // Refund SP
    const refundPct = [10, 30, 50, 70, 90][learnedLevel - 1];
    const refund = Math.floor(activeCast.spCost * refundPct / 100);
    // Note: SP was NOT consumed at cast start, so no actual refund needed
    // Cast Cancel in RO works because SP is pre-consumed in some implementations
    // In our system (SP consumed at execution), Cast Cancel simply removes the cast

    player.mana = Math.max(0, player.mana - 2); // 2 SP cost for Cast Cancel itself
    socket.emit('skill:cast_interrupted', { skillId: activeCast.skillId, reason: 'cast_cancel' });
    socket.emit('skill:used', { skillId, skillName: skill.name, level: learnedLevel, spCost: 2, remainingMana: player.mana, maxMana: player.maxMana });
    return;
}
```

**Note:** In our system, SP is consumed at execution time, not cast start. Cast Cancel therefore simply removes the active cast without needing to refund SP. The SP retention percentage is only relevant if we change to pre-consume SP at cast start (which is the canonical RO behavior for this interaction).

### 11.2 Sense — ID 1419

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 0ms |
| Target | Single enemy |
| Range | 9 cells (450 UE units) |

**Effect:** Reveals target monster's information:
- HP / Max HP
- SP / Max SP
- Element (type + level)
- Size (Small/Medium/Large)
- Race
- Hard DEF / Hard MDEF
- Base Level

**Implementation:**
```js
if (skill.name === 'sense_sage') {
    if (!targetId || !isEnemy) { socket.emit('skill:error', { message: 'Target an enemy' }); return; }
    const enemy = enemies.get(targetId);
    if (!enemy) { socket.emit('skill:error', { message: 'Enemy not found' }); return; }

    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    socket.emit('skill:sense_result', {
        targetId,
        targetName: enemy.name,
        hp: enemy.health,
        maxHp: enemy.maxHealth,
        element: enemy.element,
        size: enemy.size,
        race: enemy.race,
        hardDef: enemy.def || 0,
        hardMdef: enemy.mdef || 0,
        baseLevel: enemy.level || 1
    });
    socket.emit('skill:used', { ... });
    return;
}
```

**New Socket Event:** `skill:sense_result` -- client displays a popup with monster info.

### 11.3 Abracadabra (Hocus Pocus) — ID 1416

| Property | Value |
|----------|-------|
| Max Level | 10 |
| SP Cost | 50 (all levels) |
| Cast Time | 0ms |
| Cooldown | 3000ms |
| Target | Self |
| Catalyst | 2 Yellow Gemstones (715) |
| Prerequisites | Hindsight Lv5, Land Protector Lv1, Dispell Lv1 |

**Effect:** Casts a random skill at the caster's current skill level. The Hindsight level determines which skills can be randomly selected. Higher levels unlock more powerful potential spells.

**Exclusive effects (unique to Abracadabra):**
- **Coma** -- Reduce target's HP to 1 and SP to 0 (does not work on Boss/MVP)
- **Class Change** -- Transform target monster into a random monster
- **Mono Cell** -- Transform target monster into a Poring
- **Rejuvenation** -- Fully restore target's HP and SP
- **Grim Reaper** -- Instantly kill target monster (no EXP/loot)
- **Gravity** -- Random earthquake damage to surroundings
- **Gold Digger** -- Randomly spawn gold on the ground
- **Leveling** -- Randomly grant 1 base level (extremely rare)

**Implementation Recommendation:** Abracadabra is extremely complex and chaotic. For initial implementation, recommend a **simplified version** that randomly casts one of the player's learned offensive spells at the target. The exclusive effects (Coma, Class Change, etc.) can be added later as they require multiple new systems. Priority: LOW.

**SKILL_CATALYSTS entry:**
```js
'abracadabra':   [{ itemId: 715, quantity: 2 }],  // 2x Yellow Gemstone
```

---

## 12. Quest Skills (Create Converter / Elemental Change)

### 12.1 Create Elemental Converter — ID 1420

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 30 |
| Cast Time | 0ms |
| Target | Self |
| Type | Crafting |

**Recipes:**

| Converter Created | Materials Required |
|------------------|--------------------|
| Fire Elemental Converter | 3 Scorpion Tails + 1 Blank Scroll |
| Water Elemental Converter | 3 Snail's Shells + 1 Blank Scroll |
| Earth Elemental Converter | 3 Horns + 1 Blank Scroll |
| Wind Elemental Converter | 3 Rainbow Shells + 1 Blank Scroll |

**Elemental Converter Items:**
- Fire Converter = self-endow Fire for 5 minutes
- Water Converter = self-endow Water for 5 minutes
- Earth Converter = self-endow Earth for 5 minutes
- Wind Converter = self-endow Wind for 5 minutes

**Implementation:** Requires a crafting UI popup to select which converter to create. Server validates materials and creates the item. Priority: MEDIUM (useful for all melee classes).

### 12.2 Elemental Change — ID 1421

| Property | Value |
|----------|-------|
| Max Level | 1 (4 variants: Fire/Water/Earth/Wind) |
| SP Cost | 30 |
| Cast Time | 2000ms |
| After-Cast Delay | 1000ms |
| Target | Single enemy |
| Range | 9 cells (450 UE units) |
| Catalyst | 1 Elemental Converter (matching element) |
| Requirement | Job Level 40 |

**Effect:** Permanently changes the target monster's element to the converter's element at Level 1.

**Implementation:** This would modify `enemy.element` to `{ type: chosenElement, level: 1 }` for the duration of that enemy's life. Useful strategically but complex to implement properly. Priority: LOW.

---

## 13. New Systems Required

### 13.1 Weapon Endow System (Priority: HIGH)

**What it does:** Temporarily overrides a player's weapon element.

**Changes needed:**
1. Add `player.weaponEndow` field (tracks active endow buff)
2. Add endow buffs to buff system (`endow_fire`, `endow_water`, `endow_wind`, `endow_earth`)
3. Modify `getAttackerInfo()` to check `player.weaponEndow` before falling back to `player.weaponElement`
4. On endow buff expiry, revert to base weapon element
5. Mutual exclusion: new endow replaces existing endow
6. Add to SKILL_CATALYSTS: Red Blood (990), Crystal Blue (991), Wind of Verdure (992), Green Live (993)

**Affected files:**
- `server/src/index.js` -- skill handler, getAttackerInfo(), buff system
- `client/.../BuffBarSubsystem.cpp` -- display endow buff icon

### 13.2 Ground Effect System (Priority: HIGH)

**What it does:** Persistent area effects on the ground (Volcano/Deluge/Violent Gale/Land Protector).

**Changes needed:**
1. `activeGroundEffects` Map on server
2. Ground effect creation/expiry/overlap validation
3. Per-tick check: who is standing inside each ground effect
4. Apply stat bonuses to entities inside ground effects
5. Land Protector interaction (removes/blocks other ground effects)
6. Socket events: `ground:effect_created`, `ground:effect_expired`
7. Client-side: visual ground effect rendering (could use Niagara decals or ground projections)

**Affected files:**
- `server/src/index.js` -- new ground effect system
- `client/.../SkillVFXSubsystem.cpp` -- ground effect visuals
- New client subsystem or extend existing one for ground effect tracking

### 13.3 Magic Absorption System (Priority: MEDIUM)

**What it does:** Magic Rod absorbs incoming single-target spells.

**Changes needed:**
1. Add `magic_rod_active` buff type
2. In all magical damage paths (bolt skills, Soul Strike, etc.), check if target has `magic_rod_active`
3. If active: cancel damage, restore SP to target, broadcast absorption event
4. Duration-based (400-1200ms window)

**Affected files:**
- `server/src/index.js` -- all magic damage handlers need absorption check

### 13.4 Cast Interruption System (Priority: MEDIUM)

**What it does:** Spell Breaker interrupts enemy casting + absorbs SP.

**Changes needed:**
1. New handler that checks `activeCasts` for target
2. Removes target from `activeCasts`
3. Calculates SP absorption
4. Lv5 special: MaxHP% damage + HP drain

### 13.5 Dispell System (Priority: MEDIUM)

**What it does:** Removes all buffs from target.

**Changes needed:**
1. `UNDISPELLABLE` set of buff names that resist Dispell
2. Iterate `target.activeBuffs`, remove all dispellable buffs
3. Broadcast individual `skill:buff_removed` for each removed buff
4. Handle endow revert on dispel

### 13.6 Auto Spell System (Priority: MEDIUM)

**What it does:** Hindsight auto-casts spells on melee attack.

**Changes needed:**
1. `hindsight` buff with autocast chance and spell pool
2. Modify auto-attack tick to check for hindsight buff after each hit
3. Spell pool selection logic based on hindsight level and learned skills
4. Bolt level randomization (50/35/15% distribution)
5. 2/3 SP cost for autocasted spells
6. Instant cast, still has ACD

### 13.7 Free Cast System (Priority: LOW)

**What it does:** Allow movement while casting without interruption.

**Changes needed:**
1. Modify `player:position` handler to check `player.learnedSkills[1405]`
2. If Free Cast learned: skip `interruptCast()` on movement
3. Reduce movement speed during casting
4. Damage still interrupts (unless Phen Card equipped)

### 13.8 Ground Effect Client Rendering (Priority: LOW)

**What it does:** Visual display of ground effects on client.

**Changes needed:**
1. New ground effect handler in VFX subsystem
2. Spawn persistent decal/particle at ground position
3. Remove on expiry event
4. Color/effect varies by skill type

---

## 14. Integration Points

### 14.1 With Buff System (`ro_buff_system.js`)

New buff types needed:
- `endow_fire`, `endow_water`, `endow_wind`, `endow_earth` -- weapon element override
- `hindsight` -- autocast data
- `magic_rod_active` -- spell absorption window

### 14.2 With Combat Pipeline (`ro_damage_formulas.js`)

- `getAttackerInfo()` must check weapon endow before weapon element
- Ground effect bonuses (ATK, FLEE, MaxHP) must be included in effective stats
- Magic Rod absorption check in all magical damage paths

### 14.3 With SKILL_CATALYSTS (`index.js`)

New entries needed:
```js
'endow_blaze':    [{ itemId: 990, quantity: 1 }],
'endow_tsunami':  [{ itemId: 991, quantity: 1 }],
'endow_tornado':  [{ itemId: 992, quantity: 1 }],
'endow_quake':    [{ itemId: 993, quantity: 1 }],
'volcano':        [{ itemId: 715, quantity: 1 }],
'deluge':         [{ itemId: 715, quantity: 1 }],
'violent_gale':   [{ itemId: 715, quantity: 1 }],
'abracadabra':    [{ itemId: 715, quantity: 2 }],
// land_protector and dispell already exist
```

### 14.4 With Passive Skill System (`getPassiveSkillBonuses()`)

New passives:
- Advanced Book (1400): `bonusATK` with books, `bookAspdBonus`
- Dragonology (1407): `bonusINT`, `raceATK.dragon`, `raceResist.dragon`

### 14.5 With Cast Interruption System

- Free Cast (1405): skip `interruptCast()` on movement if learned
- Cast Cancel (1401): manually interrupt own cast with SP refund

### 14.6 With VFX System (`SkillVFXData.cpp`)

New VFX configs needed for all 22 skills. Templates:
- Endow skills: `SelfBuff` (elemental aura on target)
- Volcano/Deluge/Violent Gale: `GroundPersistent` (elemental ground patch)
- Land Protector: `GroundPersistent` (translucent barrier)
- Earth Spike: `BoltFromSky` (earth spikes from below)
- Heaven's Drive: `GroundAoERain` (earth eruption)
- Dispell: `TargetDebuff` (dispel flash)
- Hindsight: `SelfBuff` (auto-spell aura)
- Magic Rod: `SelfBuff` (absorption shield)
- Spell Breaker: `Projectile` (disruption beam)

---

## 15. Implementation Priority

### Phase 1 (HIGH) -- Core Support Skills
1. **Endow skills** (1408-1411) -- weapon endow system + catalysts
2. **Earth Spike Sage** (1417) -- reuse Wizard handler
3. **Heaven's Drive Sage** (1418) -- reuse Wizard handler
4. **Passive: Advanced Book** (1400) -- add to getPassiveSkillBonuses()
5. **Passive: Dragonology** (1407) -- add to getPassiveSkillBonuses()
6. **Sense** (1419) -- simple info dump skill

### Phase 2 (MEDIUM) -- Anti-Mage + Autocast
7. **Dispell** (1403) -- buff removal system
8. **Spell Breaker** (1406) -- cast interruption
9. **Cast Cancel** (1401) -- self-cast cancel
10. **Hindsight** (1402) -- autocast system (most complex)

### Phase 3 (MEDIUM-LOW) -- Ground Effects
11. **Volcano** (1412) -- requires ground effect system
12. **Deluge** (1413)
13. **Violent Gale** (1414)
14. **Land Protector** (1415)

### Phase 4 (LOW) -- Advanced / Quest
15. **Free Cast** (1405) -- movement during casting
16. **Magic Rod** (1404) -- spell absorption
17. **Create Converter** (1420) -- crafting system
18. **Elemental Change** (1421) -- monster element change
19. **Abracadabra** (1416) -- random skill cast (extremely complex)

---

## 16. Constants and Data Tables

### 16.1 Hindsight Autocast Chances (Pre-Renewal)

```js
const HINDSIGHT_AUTOCAST_CHANCE = [7, 9, 11, 13, 15, 17, 20, 22, 23, 25];
```

### 16.2 Hindsight Spell Pool

```js
const HINDSIGHT_SPELL_POOL = {
    1: [200, 201, 202],                          // Cold/Fire/Lightning Bolt
    4: [200, 201, 202, 210, 207],                // + Soul Strike, Fire Ball
    7: [200, 201, 202, 210, 207, 208, 804],      // + Frost Diver, Earth Spike (wizard ver)
    10: [200, 201, 202, 210, 207, 208, 804, 212, 805] // + Thunderstorm, Heaven's Drive (wiz ver)
};
// Level 1-3: pool from key 1, Level 4-6: pool from key 4, etc.
```

### 16.3 Hindsight Bolt Level Distribution

```js
const HINDSIGHT_BOLT_LEVEL_WEIGHTS = [
    { level: 1, weight: 50 },
    { level: 2, weight: 35 },
    { level: 3, weight: 15 }
];
```

### 16.4 Endow Success Rates (Pre-Renewal)

```js
const ENDOW_SUCCESS_RATE = [70, 80, 90, 100, 100];
```

### 16.5 Dispell Success Rates

```js
const DISPELL_SUCCESS_RATE = [60, 70, 80, 90, 100];
```

### 16.6 Dragonology INT Bonuses

```js
const DRAGONOLOGY_INT_BONUS = [1, 1, 1, 2, 3];
```

### 16.7 Ground Effect Radius by Skill/Level

```js
const GROUND_EFFECT_RADIUS = {
    volcano: 350,      // 7x7 all levels
    deluge: 350,        // 7x7 all levels
    violent_gale: 350,  // 7x7 all levels
    land_protector: {   // Scales with level
        1: 350, 2: 350, // 7x7
        3: 450, 4: 450, // 9x9
        5: 550           // 11x11
    }
};
```

### 16.8 Ground Effect Stat Bonuses

```js
const VOLCANO_FIRE_DAMAGE_BONUS = [10, 14, 17, 19, 20]; // %
const VOLCANO_ATK_BONUS = [10, 15, 20, 25, 30]; // flat

const DELUGE_WATER_DAMAGE_BONUS = [10, 14, 17, 19, 20]; // %
const DELUGE_MAXHP_BONUS = [5, 9, 12, 14, 15]; // %

const VIOLENT_GALE_WIND_DAMAGE_BONUS = [10, 14, 17, 19, 20]; // %
const VIOLENT_GALE_FLEE_BONUS = [3, 6, 9, 12, 15]; // flat
```

### 16.9 Catalyst Item IDs

```js
const SAGE_CATALYSTS = {
    RED_BLOOD: 990,
    CRYSTAL_BLUE: 991,
    WIND_OF_VERDURE: 992,
    GREEN_LIVE: 993,
    YELLOW_GEMSTONE: 715,
    BLUE_GEMSTONE: 717,
    BLANK_SCROLL: 7433,  // For Create Converter
    SCORPION_TAIL: 904,
    SNAIL_SHELL: 946,
    HORN: 947,
    RAINBOW_SHELL: 1013
};
```

---

## 17. Current Codebase Status

### 17.1 Skill Definitions -- COMPLETE

All 22 Sage skills are already defined in `server/src/ro_skill_data_2nd.js` (IDs 1400-1421). The definitions include correct:
- Skill IDs, names, display names
- Class ID (`sage`)
- Max levels
- Prerequisites (both Mage and Sage internal)
- Tree positions (treeRow/treeCol)
- Type/targetType
- Element
- Level arrays with SP costs, cast times, cooldowns, effectValues, durations

### 17.2 Skill Definitions -- CORRECTIONS NEEDED

| Skill | Field | Current | Canonical | Fix |
|-------|-------|---------|-----------|-----|
| Volcano (1412) | SP Cost | `40+i*2` (40-48) | `48-i*2` (48-40) | Reverse the formula |
| Deluge (1413) | SP Cost | `40+i*2` (40-48) | `48-i*2` (48-40) | Reverse the formula |
| Violent Gale (1414) | SP Cost | `40+i*2` (40-48) | `48-i*2` (48-40) | Reverse the formula |
| Volcano/Deluge/VGale | Range | 450 | 100 (2 cells) | Change to 100 |
| Land Protector (1415) | SP Cost | `50+i*4` (50-66) | `70-i*4` (66-50) | Reverse the formula |
| Land Protector (1415) | Range | 450 | 150 (3 cells) | Change to 150 |
| Hindsight (1402) | Cast Time | 2000ms | 3000ms (pre-renewal) | Update to 3000 |
| Heaven's Drive Sage (1418) | SP Cost | `26+i*2` (26-34) | `24+i*4` (28-44) | Update formula |
| Advanced Book (1400) | Max Level | 10 | 10 (iRO Wiki says 10) | Correct |
| Endow skills (1408-1411) | Duration | `1200000+i*120000` | 20 min flat or 20-30 min | Audit needed |
| Endow skills (1408-1411) | effectValue | `70+i*6` | Success rates: 70/80/90/100/100 | Encode properly |

### 17.3 Skill Handlers -- NONE IMPLEMENTED

No Sage skill handlers exist in `server/src/index.js`. All 22 skills need handlers.

### 17.4 Passive Bonuses -- NONE IMPLEMENTED

`getPassiveSkillBonuses()` has no entries for:
- Advanced Book (1400) -- ATK + ASPD with Books
- Free Cast (1405) -- movement while casting
- Dragonology (1407) -- INT + Dragon bonuses

### 17.5 SKILL_CATALYSTS -- PARTIALLY IMPLEMENTED

Already in SKILL_CATALYSTS:
- `dispell`: Yellow Gemstone
- `land_protector`: Blue + Yellow Gemstone

Missing from SKILL_CATALYSTS:
- `endow_blaze`: Red Blood (990)
- `endow_tsunami`: Crystal Blue (991)
- `endow_tornado`: Wind of Verdure (992)
- `endow_quake`: Green Live (993)
- `volcano`: Yellow Gemstone (715)
- `deluge`: Yellow Gemstone (715)
- `violent_gale`: Yellow Gemstone (715)
- `abracadabra`: 2x Yellow Gemstone (715)

### 17.6 CLASS_PROGRESSION -- CORRECT

`sage: ['novice', 'mage', 'sage']` is already defined in `ro_skill_data.js`.

---

## Appendix A: Endow Skill Data Audit (Pre-Renewal vs Current)

### Endow Duration Analysis

Multiple sources disagree on pre-renewal endow duration:
- **RateMyServer (Pre-Re)**: Lv1-4 = 20 min, Lv5 = 30 min
- **iRO Wiki (Renewal)**: 5 * (SkillLv + 1) min = 10/15/20/25/30 min
- **RagnaCloneDocs**: 20-60s per level (seems wrong, likely 20-60 min typo)

**Recommendation:** Use RateMyServer Pre-Re values (Lv1-4 = 1,200,000ms, Lv5 = 1,800,000ms). The current definition uses `1200000+i*120000` which gives 1.2M/1.32M/1.44M/1.56M/1.68M -- this is close but not exactly right.

**Corrected formula:**
```js
duration: learnedLevel < 5 ? 1200000 : 1800000
```

### Endow Success Rate Encoding

The current `effectValue: 70+i*6` gives 70, 76, 82, 88, 94. The canonical success rates are 70, 80, 90, 100, 100. The effectValue should not encode success rate -- it should be handled separately in the handler.

---

## Appendix B: Scholar (Professor) Skills Preview

The transcendent version of Sage is Scholar (Professor). It inherits all Mage + Sage skills and adds:
- **Health Conversion** -- Convert 10% MaxHP to SP
- **Soul Change** -- Swap SP pool with target ally
- **Soul Burn** -- Drain target's SP, deal MATK damage if fully drained
- **Mind Breaker** -- Increase target MATK, decrease target MDEF
- **Spider Web** -- Immobilize target on ground (Fire vulnerable)
- **Fiber Lock** -- Extend web duration (transcendent upgrade)
- **Wall of Fog** -- AoE that reduces ranged damage and HIT
- **Double Casting** -- Chance to double-cast bolt spells
- **Indulge** -- Convert HP to SP (same as Health Conversion)
- **Memorize** -- Halve all cast times for 5 casts

These are NOT part of Sage implementation but are noted for future planning. Scholar skill IDs would be in a separate range (e.g., 2400+).

---

## Appendix C: Source Cross-Reference

| Skill | iRO Wiki | RateMyServer | rAthena Pre-Re | RagnaCloneDocs |
|-------|----------|-------------|----------------|----------------|
| Advanced Book | Lv10, +3ATK/+0.5%ASPD | Lv10, same | Lv10, same | Lv5 (typo?) |
| Cast Cancel | Lv5, 10-90% refund | Lv5, 90-10% (reversed) | -- | Lv5, 65-90% |
| Free Cast | Lv10, 30-75% move | Lv10, same | -- | Lv10, 30-100% |
| Magic Rod | Lv5, 400-1200ms, 20-100% | Lv5, same | -- | Lv5 |
| Spell Breaker | Lv5, 0-100% absorb | Lv5, same | -- | Lv5 |
| Hindsight | Lv10, 2-20% (renewal) | Lv10, 7-25% (pre-re) | -- | Lv10, 300s |
| Endow skills | Lv5, 10-30 min (renewal) | Lv5, 20-30 min (pre-re) | -- | Lv5, 20-60s |
| Volcano | Lv5, +10-20% fire | Lv5, same | -- | Lv5, +10-50% |
| Land Protector | Lv5, 7x7-11x11 | Lv5, same | -- | Lv5 |
| Dispell | Lv5, 60-100% | Lv5, same | -- | Lv5, same |
| Earth Spike | Lv5, 200%/hit | Lv5, same | -- | -- |
| Heaven's Drive | Lv5, 125%/hit | Lv5, same | -- | -- |
| Dragonology | Lv5, +20% dragon | Lv5, same | -- | Lv5, +3 INT |

**Most authoritative source for pre-renewal:** RateMyServer (Pre-Re database) + rAthena pre-renewal skill_db.yml

---

## Appendix D: Full Hindsight Spell Pool Reference

### Available Spells by Hindsight Level (Pre-Renewal)

| HL | New Spells Added | Cumulative Pool |
|----|-----------------|-----------------|
| 1 | Napalm Beat (Lv1-3) | Napalm Beat |
| 2 | Cold/Fire/Lightning Bolt (Lv1) | NB, CB, FB, LB |
| 3 | Cold/Fire/Lightning Bolt (Lv2) | NB, CB, FB, LB |
| 4 | Cold/Fire/Lightning Bolt (Lv3) | NB, CB, FB, LB |
| 5 | Soul Strike (Lv1) | NB, CB, FB, LB, SS |
| 6 | Soul Strike (Lv2) | NB, CB, FB, LB, SS |
| 7 | Soul Strike (Lv3) | NB, CB, FB, LB, SS |
| 8 | Fire Ball (Lv1) | NB, CB, FB, LB, SS, FBall |
| 9 | Fire Ball (Lv2) | NB, CB, FB, LB, SS, FBall |
| 10 | Frost Diver (Lv1) | NB, CB, FB, LB, SS, FBall, FD |

**Note:** The "Lv" after the spell name indicates the maximum autocast level for that spell, not a prerequisite. The actual level cast is determined by the bolt level distribution (50/35/15 for Lv1/2/3) capped by the player's learned level.

**Alternative (Renewal) spell pool** is significantly different -- uses Lv1-3 bolts at HL1-3, Soul Strike/Fire Ball at HL4-6, Frost Diver/Earth Spike at HL7-9, Thunderstorm/Heaven's Drive at HL10. The pre-renewal version from RateMyServer is authoritative for this project.
