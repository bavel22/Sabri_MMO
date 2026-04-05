# Priest Class — Complete Pre-Renewal Research & Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Priest_Skills_Audit](Priest_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE — Ready for implementation
**Base Class:** Acolyte (inherits all 15 Acolyte skills)
**Transcendent:** High Priest (future phase)
**Alternate 2nd Class:** Monk (separate branch, already defined in `ro_skill_data_2nd.js`)

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Existing Implementation Status](#existing-implementation-status)
3. [Complete Skill List](#complete-skill-list)
4. [Skill Tree & Prerequisites](#skill-tree--prerequisites)
5. [Detailed Skill Specifications](#detailed-skill-specifications)
   - [Mace Mastery](#1-mace-mastery-id-1008--passive)
   - [Impositio Manus](#2-impositio-manus-id-1009--active-supportive)
   - [Suffragium](#3-suffragium-id-1010--active-supportive)
   - [Aspersio](#4-aspersio-id-1011--active-supportive)
   - [B.S. Sacramenti](#5-bs-sacramenti-id-1012--active-supportive)
   - [Sanctuary](#6-sanctuary-id-1000--active-ground-aoe)
   - [Kyrie Eleison](#7-kyrie-eleison-id-1001--active-supportive)
   - [Magnificat](#8-magnificat-id-1002--active-supportive)
   - [Gloria](#9-gloria-id-1003--active-supportive)
   - [Resurrection](#10-resurrection-id-1004--active-supportive)
   - [Magnus Exorcismus](#11-magnus-exorcismus-id-1005--active-offensive)
   - [Turn Undead](#12-turn-undead-id-1006--active-offensive)
   - [Lex Aeterna](#13-lex-aeterna-id-1007--active-debuff)
   - [Lex Divina](#14-lex-divina-id-1015--active-debuff)
   - [Status Recovery](#15-status-recovery-id-1014--active-supportive)
   - [Slow Poison](#16-slow-poison-id-1013--active-supportive)
   - [Increase SP Recovery (Priest)](#17-increase-sp-recovery-priest-id-1016--passive)
   - [Safety Wall (Priest)](#18-safety-wall-priest-id-1017--active-ground)
   - [Redemptio](#19-redemptio-id-1018--active-quest-skill)
6. [New Systems Required](#new-systems-required)
7. [Skill Data Corrections](#skill-data-corrections)
8. [Implementation Priority](#implementation-priority)
9. [Integration Points](#integration-points)
10. [High Priest Preview (Future Phase)](#high-priest-preview-future-phase)
11. [Sources](#sources)

---

## Class Overview

The Priest is the primary healer and support class in Ragnarok Online Classic (Pre-Renewal). Priests provide:

- **Party healing** via Heal, Sanctuary (ground AoE heal)
- **Damage mitigation** via Kyrie Eleison (barrier), Safety Wall (melee block)
- **Offensive holy magic** via Magnus Exorcismus (ground AoE vs Undead/Demon), Turn Undead (instant kill chance), Holy Light (inherited from Acolyte)
- **Buff support** via Blessing, Increase AGI (inherited), Impositio Manus (ATK), Gloria (LUK), Magnificat (SP regen), Aspersio (Holy weapon), Suffragium (cast time reduction)
- **Debuff/utility** via Lex Aeterna (double next damage), Lex Divina (silence), Status Recovery, Slow Poison
- **Resurrection** of dead players
- **Passive ATK** with Mace Mastery

**Primary stats:** INT (heal power, MATK, SP pool), VIT (survivability), DEX (cast time reduction)

**Weapon types:** Maces, Rods/Staves, Books (2nd class onward)

**Skill points:** 49 job levels = 49 skill points for Priest skills (plus Acolyte's 49)

---

## Existing Implementation Status

### Skill Definitions (ro_skill_data_2nd.js)

All 19 Priest skills are already defined in `server/src/ro_skill_data_2nd.js` with IDs 1000-1018. The class progression chain `'priest': ['novice', 'acolyte', 'priest']` is correctly defined in `ro_skill_data.js`.

### Server Handlers (index.js)

**NO Priest skill handlers exist in `index.js` yet.** All 19 skills are definition-only with no execution logic. The skill definitions appear in the client skill tree and can be learned, but using any Priest skill will silently fail (no handler matched in the `skill:use` switch block).

### Catalyst System

The `SKILL_CATALYSTS` constant in `index.js` already includes:
- `'sanctuary': [{ itemId: 717, quantity: 1 }]` (Blue Gemstone)
- `'resurrection': [{ itemId: 717, quantity: 1 }]` (Blue Gemstone)
- `'magnus_exorcismus': [{ itemId: 717, quantity: 1 }]` (Blue Gemstone)
- `'aspersio': [{ itemId: 523, quantity: 1 }]` (Holy Water)

### Missing Catalysts

The following skills need catalyst entries added:
- `'safety_wall_priest': [{ itemId: 717, quantity: 1 }]` (Blue Gemstone)
- `'bs_sacramenti'` needs Holy Water (ID 523) — canonically requires 1 Holy Water

---

## Complete Skill List

| ID | Name | Type | Max Lv | Target | Element | Key Mechanic |
|----|------|------|--------|--------|---------|-------------|
| 1000 | Sanctuary | Active/Ground | 10 | Ground 5x5 | Holy | Heal zone, damages Undead |
| 1001 | Kyrie Eleison | Active/Single | 10 | Self/Ally | Holy | Damage barrier (MaxHP%) |
| 1002 | Magnificat | Active/Self | 5 | Self (Party) | Holy | Double SP regen |
| 1003 | Gloria | Active/Self | 5 | Self (Party) | Holy | +30 LUK |
| 1004 | Resurrection | Active/Single | 4 | Dead Player | Holy | Revive with HP%, Blue Gem |
| 1005 | Magnus Exorcismus | Active/Ground | 10 | Ground 7x7 | Holy | Multi-wave vs Undead/Demon, Blue Gem |
| 1006 | Turn Undead | Active/Single | 10 | Enemy | Holy | Instant-kill Undead chance |
| 1007 | Lex Aeterna | Active/Single | 1 | Enemy | Neutral | Double next damage |
| 1008 | Mace Mastery | Passive | 10 | Self | Neutral | +3 ATK/lv with Maces |
| 1009 | Impositio Manus | Active/Single | 5 | Self/Ally | Neutral | +5 ATK/lv buff |
| 1010 | Suffragium | Active/Single | 3 | Ally (NOT self) | Neutral | -15/30/45% next cast time |
| 1011 | Aspersio | Active/Single | 5 | Self/Ally | Holy | Holy weapon endow, Holy Water |
| 1012 | B.S. Sacramenti | Active/Ground | 5 | Ground 3x3 | Holy | Holy armor endow, needs 2 Aco classes |
| 1013 | Slow Poison | Active/Single | 4 | Self/Ally | Neutral | Pause poison HP drain |
| 1014 | Status Recovery | Active/Single | 1 | Self/Ally | Neutral | Cure Frozen/Stone/Stun |
| 1015 | Lex Divina | Active/Single | 10 | Enemy | Neutral | Silence, or cure silence |
| 1016 | Increase SP Recovery | Passive | 10 | Self | Neutral | +SP regen |
| 1017 | Safety Wall (Priest) | Active/Ground | 10 | Ground | Neutral | Block melee hits, Blue Gem |
| 1018 | Redemptio | Active/Self | 1 | Self (Party) | Holy | Mass resurrect, quest skill |

---

## Skill Tree & Prerequisites

### Acolyte Prerequisites (inherited, already implemented)

```
[Acolyte Tree - Row/Col layout]
(0,0) Heal             -> (1,0) Cure [Heal 2]
(0,1) Inc AGI [Heal 3] -> (1,1) Dec AGI [Inc AGI 1]
(0,2) Div Protection   -> (1,2) Blessing [Div Prot 5]
                        -> (2,2) Angelus [Div Prot 3]
(0,3) Demon Bane [Div Prot 3] -> Signum Crucis [Demon Bane 3]
(0,4) Aqua Benedicta
(2,0) Ruwach -> (3,0) Teleport [Ruwach 1] -> (4,0) Warp Portal [Teleport 2]
             -> (5,0) Pneuma [Warp Portal 4]
(5,2) Holy Light (quest skill, no prereqs)
```

### Priest Skill Tree (rAthena canonical prerequisites)

```
[Priest Tree - Row/Col layout]

Row 0:
  (0,0) Sanctuary [Heal 1 (Aco)]
  (0,1) Kyrie Eleison [Angelus 2 (Aco)]
  (0,2) Resurrection [Heal 4 (Aco), Inc SP Rec 4 (Aco)]
  (0,3) Mace Mastery [no prereqs]

Row 1:
  (1,0) Magnificat [no prereqs]
  (1,1) Gloria [Kyrie 4, Magnificat 3]
  (1,2) Lex Aeterna [Lex Divina 5]
  (1,3) Impositio Manus [Mace Mastery 3]

Row 2:
  (2,0) Magnus Exorcismus [Turn Undead 3, Lex Aeterna 1, Safety Wall 1 (Mage or Priest)]
  (2,1) Turn Undead [Resurrection 1, Lex Divina 1]
  (2,2) Suffragium [Impositio 2]
  (2,3) Aspersio [Aqua Benedicta 1 (Aco), Impositio 3]

Row 3:
  (3,0) Slow Poison [no prereqs]
  (3,1) Status Recovery [no prereqs]
  (3,2) Lex Divina [Ruwach 1 (Aco)]
  (3,3) B.S. Sacramenti [Aspersio 3, Gloria 3]

Row 4:
  (4,0) Increase SP Recovery [no prereqs]
  (4,1) Safety Wall (Priest) [Inc SP Recovery 3]
  (4,2) Redemptio [quest skill, no prereqs]
```

### Key Prerequisite Chains

**Heal Chain:** Heal (Aco) -> Sanctuary -> (no further Priest prereqs)

**Defense Chain:** Angelus (Aco) -> Kyrie Eleison -> Gloria (also needs Magnificat 3)

**Exorcism Chain:** Resurrection -> Turn Undead -> Magnus Exorcismus (also needs Lex Aeterna 1 + Safety Wall 1)

**Debuff Chain:** Ruwach (Aco) -> Lex Divina -> Lex Aeterna -> Magnus Exorcismus

**ATK Support Chain:** Mace Mastery -> Impositio Manus -> Suffragium / Aspersio -> B.S. Sacramenti

**SP/Defense Chain:** Inc SP Recovery -> Safety Wall (Priest)

---

## Detailed Skill Specifications

---

### 1. MACE MASTERY (ID 1008) — Passive

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 65 (PR_MACEMASTERY) | rAthena |
| ATK Bonus | +3 per level (+3 to +30) | iRO Wiki Classic, RateMyServer |
| ATK Type | Weapon mastery (flat, ignores armor DEF, bypasses multipliers) | rAthena |
| Weapon Restriction | Mace weapons only | All sources |
| Prerequisites | None | All sources |

#### Current Implementation Status: DEFINITION CORRECT, NO HANDLER

The skill definition in `ro_skill_data_2nd.js` is correct: `effectValue: (i+1)*3` gives +3 to +30.

#### Implementation Required

Add to `getPassiveSkillBonuses()` in `index.js`:

```js
const maceMasteryLv = learned[1008] || 0;
if (maceMasteryLv > 0 && player.weaponType === 'mace') {
    bonuses.bonusATK += maceMasteryLv * 3;
}
```

This follows the exact pattern of Sword Mastery (ID 100, +4 ATK/lv with 1H swords).

---

### 2. IMPOSITIO MANUS (ID 1009) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 5 | All sources |
| rAthena ID | 66 (PR_IMPOSITIO) | rAthena |
| Target | Self or single ally | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| ATK Bonus | +5, +10, +15, +20, +25 | iRO Wiki Classic, RateMyServer |
| Duration | 60 seconds (all levels) | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Prerequisites | Mace Mastery Lv3 | All sources |

**SP Cost per Level:**

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| SP | 13 | 16 | 19 | 22 | 25 |

#### Current Definition Status: CORRECT

`spCost: 13+i*3` gives 13, 16, 19, 22, 25. `effectValue: 5+i*5` gives 5, 10, 15, 20, 25. Duration is 60000ms. No cast time.

#### Implementation Required

**New buff type: `impositio_manus`**

```js
// Buff fields:
{
    name: 'impositio_manus',
    bonusATK: effectVal,  // +5 to +25 flat ATK
    duration: 60000
}
```

The `bonusATK` should be applied in `getBuffStatModifiers()` and added to the player's effective ATK. This is a flat ATK bonus added at the mastery/bonus ATK step (like Weapon Mastery passives), meaning it bypasses armor DEF reduction.

**Handler pattern:** Standard buff template. Apply to self or target player. Overwrite existing `impositio_manus` buff on recast.

---

### 3. SUFFRAGIUM (ID 1010) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 3 | All sources |
| rAthena ID | 67 (PR_SUFFRAGIUM) | rAthena |
| Target | Single ally only (CANNOT cast on self) | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | None | iRO Wiki Classic |
| Prerequisites | Impositio Manus Lv2 | All sources |
| Special | Only affects NEXT spell cast, then removed | All sources |

**Per-Level Stats:**

| Lv | SP | Cast Reduction | Duration |
|----|----|---------------|----------|
| 1 | 8 | -15% | 30s |
| 2 | 8 | -30% | 20s |
| 3 | 8 | -45% | 10s |

Note: Higher levels have SHORTER duration because the reduction is more powerful.

#### Current Definition Status: CORRECT

Manual levels array matches perfectly.

#### Implementation Required

**New buff type: `suffragium`**

```js
{
    name: 'suffragium',
    castTimeReduction: effectVal,  // 15, 30, or 45 (percentage)
    duration: duration,            // 30000, 20000, or 10000
    consumeOnCast: true            // Removed after first spell cast
}
```

**Integration with cast time system:**

In `calculateActualCastTime()` or the cast time block of `skill:use`, check if the player has an active `suffragium` buff. If so:

```js
let actualCastTime = baseCastTime * (1 - effectiveDex / 150);
const suffBuff = getActiveBuff(player, 'suffragium');
if (suffBuff) {
    actualCastTime = Math.floor(actualCastTime * (100 - suffBuff.castTimeReduction) / 100);
    removeBuff(player, 'suffragium');  // Consumed on use
    broadcastToZone(zone, 'skill:buff_removed', { ... });
}
```

**Self-cast check:** Handler MUST reject if `targetId === characterId`. Emit `skill:error: 'Cannot cast on self'`.

---

### 4. ASPERSIO (ID 1011) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 5 | All sources |
| rAthena ID | 68 (PR_ASPERSIO) | rAthena |
| Target | Self or single ally | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Catalyst | 1 Holy Water (ID 523) | All sources |
| Cast Time | 1 second (RateMyServer) / 2 seconds (some sources) | Varies |
| After-Cast Delay | 0.5-1 second | iRO Wiki Classic |
| Prerequisites | Aqua Benedicta Lv1 (Aco), Impositio Manus Lv3 | All sources |
| Effect | Endow target's weapon with Holy element | All sources |

**Per-Level Stats:**

| Lv | SP | Duration |
|----|------|----------|
| 1 | 14 | 60s |
| 2 | 18 | 90s |
| 3 | 22 | 120s |
| 4 | 26 | 150s |
| 5 | 30 | 180s |

#### Current Definition Status: NEEDS CORRECTION

The current definition has `spCost: 14+i*4` which gives 14, 18, 22, 26, 30. This matches canonical values.

However, `castTime: 2000` — some sources say 1s, some say 2s. The RagnaCloneDocs don't specify. **Keep 2000ms as defined** (matches current definition and is conservative).

Duration `60000+i*30000` gives 60, 90, 120, 150, 180 seconds. **CORRECT.**

#### Implementation Required

**New buff type: `aspersio`**

```js
{
    name: 'aspersio',
    weaponElement: 'holy',
    duration: duration
}
```

**Integration with combat system:**

When `aspersio` buff is active, the player's weapon element becomes `holy` for all auto-attacks and weapon-element skills. In `getAttackerInfo()`:

```js
// Check for element endow buffs
const aspersioBuff = getActiveBuff(player, 'aspersio');
if (aspersioBuff) {
    attackerInfo.weaponElement = 'holy';
}
```

This overrides the weapon's natural element (from DB/equipment). The endow is removed when the buff expires. Aspersio should cancel other element endows (future: Endow Blaze, Endow Tsunami, etc.).

**Catalyst consumption:** Already registered in `SKILL_CATALYSTS` as `'aspersio': [{ itemId: 523, quantity: 1 }]`.

**Undead element caster:** If the target has Undead element armor (Evil Druid card), Aspersio fails and deals Holy damage to them instead. This is a unique edge case for future implementation.

---

### 5. B.S. SACRAMENTI (ID 1012) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff (Ground) | All sources |
| Max Level | 5 | All sources |
| rAthena ID | 69 (PR_BENEDICTIO) | rAthena |
| Target | Ground 3x3 | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Cast Time | 3 seconds | iRO Wiki Classic |
| SP Cost | 20 (all levels) | All sources |
| Prerequisites | Aspersio Lv3, Gloria Lv3 | All sources |
| Requirement | 2 Acolyte-class characters standing adjacent to caster | All sources |
| Effect | Endow armor of all players in 3x3 with Holy element | All sources |

**Per-Level Stats:**

| Lv | SP | Duration |
|----|----|----------|
| 1 | 20 | 40s |
| 2 | 20 | 80s |
| 3 | 20 | 120s |
| 4 | 20 | 160s |
| 5 | 20 | 200s |

#### Current Definition Status: CORRECT

`spCost: 20`, `castTime: 3000`, `duration: 40000+i*40000` gives 40, 80, 120, 160, 200 seconds.

#### Implementation Required

**New buff type: `bs_sacramenti`**

```js
{
    name: 'bs_sacramenti',
    armorElement: { type: 'holy', level: 1 },
    duration: duration
}
```

**Requirement check:** Needs 2 Acolyte-class players standing adjacent (within ~100 UE units) to the caster. This is a unique mechanic that requires:
1. Scan nearby players
2. Check if 2+ are Acolyte/Priest/Monk/High Priest/Champion class
3. If not met: `skill:error: 'Need 2 Acolyte-class members nearby'`

**DEFER the 2-Acolyte requirement** until party system exists. For now, allow solo casting (simplified).

**Integration:** When active, changes the player's `armorElement` to `{ type: 'holy', level: 1 }` for damage calculation. Holy armor is strong vs Shadow/Undead attacks but weak vs Shadow element.

---

### 6. SANCTUARY (ID 1000) — Active, Ground AoE

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Ground AoE (persistent healing zone) | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 70 (PR_SANCTUARY) | rAthena |
| Target | Ground 5x5 (250 UE units radius) | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Cast Time | 5 seconds (variable, reduced by DEX) | All sources |
| After-Cast Delay | 2 seconds | iRO Wiki Classic |
| Catalyst | 1 Blue Gemstone (ID 717) | All sources |
| Prerequisites | Heal Lv1 (Acolyte) | All sources |
| Heal Interval | 1 tick per second | All sources |
| Max Heal Targets | Level-dependent (4-13) | iRO Wiki Classic |
| Undead Damage | Half of heal value, Holy element, 2-cell knockback | All sources |

**Per-Level Stats (canonical rAthena + iRO Wiki):**

| Lv | SP | Heal/Tick | Duration (s) | Target Limit |
|----|----|-----------|--------------|----|
| 1 | 15 | 100 | 4 | 4 |
| 2 | 18 | 200 | 7 | 5 |
| 3 | 21 | 300 | 10 | 6 |
| 4 | 24 | 400 | 13 | 7 |
| 5 | 27 | 500 | 16 | 8 |
| 6 | 30 | 600 | 19 | 9 |
| 7 | 33 | 777 | 22 | 10 |
| 8 | 36 | 777 | 25 | 11 |
| 9 | 39 | 777 | 28 | 12 |
| 10 | 42 | 777 | 31 | 13 |

Note: Heal value caps at 777 at Lv7 and does not increase further.

#### Current Definition Status: NEEDS CORRECTION

- `spCost: 15+i*3` gives 15, 18, 21... **CORRECT**
- `effectValue: 100+i*100` gives 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 — **WRONG for Lv7+** (should cap at 777)
- `duration: 4000+i*1000` gives 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 seconds — **WRONG** (should be 4, 7, 10, 13, 16, 19, 22, 25, 28, 31)
- `castTime: 5000` — **CORRECT**
- Missing `afterCastDelay` — should be 2000ms

#### Implementation Required

**NEW SYSTEM: Ground AoE Heal Zone**

This is the first "persistent ground heal" effect in the project. Requires a new ground effect type with periodic heal ticks.

Ground effect structure:
```js
{
    type: 'sanctuary',
    x: groundX, y: groundY, z: groundZ,
    radius: 250,           // 5x5 cells
    healPerTick: effectVal, // 100-777
    tickInterval: 1000,     // 1 second
    lastTickTime: Date.now(),
    maxTargets: 4 + learnedLevel - 1,  // 4-13
    expiresAt: Date.now() + duration,
    casterId: characterId,
    casterName: player.characterName,
    zone: zone
}
```

**Tick logic (in ground effect tick loop, 1000ms):**

```js
for each sanctuary ground effect:
    if now >= lastTickTime + tickInterval:
        // 1. Find all alive players in radius
        // 2. Sort by distance (closest first)
        // 3. Heal up to maxTargets players
        for each targetPlayer (up to maxTargets):
            const healed = Math.min(healPerTick, target.maxHealth - target.health);
            target.health = Math.min(target.maxHealth, target.health + healed);
            broadcastToZone(zone, 'skill:effect_damage', {
                hitType: 'heal', healAmount: healed, damage: 0, ...
            });

        // 4. Find Undead element AND Demon race enemies in radius
        for each enemy in radius:
            if (enemy.element.type === 'undead' || enemy.race === 'demon'):
                const holyDmg = Math.floor(healPerTick / 2);
                // Apply Holy element modifier
                enemy.health = Math.max(0, enemy.health - holyDmg);
                // Knockback 2 cells away from center
                broadcastToZone(zone, 'skill:effect_damage', {
                    hitType: 'magical', damage: holyDmg, element: 'holy', ...
                });

        lastTickTime = now;
```

**Overlap rule:** Creating a new Sanctuary replaces the caster's previous one (max 1 per caster). Cannot overlap with existing Safety Wall.

---

### 7. KYRIE ELEISON (ID 1001) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 73 (PR_KYRIE) | rAthena |
| Target | Self or single ally | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Cast Time | 2 seconds (variable, reduced by DEX) | All sources |
| After-Cast Delay | 2 seconds | iRO Wiki Classic |
| Duration | 120 seconds (all levels) | All sources |
| Prerequisites | Angelus Lv2 (Acolyte) | All sources |

**Per-Level Stats:**

| Lv | SP | MaxHP% Shield | Max Hits Blocked |
|----|-----|--------------|-----------------|
| 1 | 20 | 12% | 5 |
| 2 | 20 | 14% | 6 |
| 3 | 20 | 16% | 6 |
| 4 | 25 | 18% | 7 |
| 5 | 25 | 20% | 7 |
| 6 | 25 | 22% | 8 |
| 7 | 30 | 24% | 8 |
| 8 | 30 | 26% | 9 |
| 9 | 30 | 28% | 9 |
| 10 | 35 | 30% | 10 |

**SP Formula:** `20 + 5 * floor((level - 1) / 3)` = 20, 20, 20, 25, 25, 25, 30, 30, 30, 35

**Barrier Formula:** `MaxHP * (10 + 2 * SkillLevel) / 100`

**Hits Formula:** `floor(SkillLevel / 2) + 5` = 5, 6, 6, 7, 7, 8, 8, 9, 9, 10

#### Current Definition Status: NEEDS CORRECTION

- `spCost: 20+i*2` gives 20, 22, 24, 26, 28, 30, 32, 34, 36, 38 — **WRONG** (should be 20, 20, 20, 25, 25, 25, 30, 30, 30, 35)
- `effectValue: 12+i*2` gives 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 — **CORRECT** (MaxHP% shield)
- `castTime: 2000` — **CORRECT**
- `duration: 120000` — **CORRECT**
- Missing after-cast delay (should be 2000ms)
- Missing max hits data (needs separate field or calculated in handler)

#### Implementation Required

**NEW SYSTEM: Damage Absorb Barrier**

```js
// New buff type: 'kyrie_eleison'
{
    name: 'kyrie_eleison',
    barrierHP: Math.floor(target.maxHealth * effectVal / 100),  // MaxHP% shield
    maxHits: Math.floor(learnedLevel / 2) + 5,                 // 5-10 hits
    hitsRemaining: maxHits,
    duration: 120000
}
```

**Integration with damage pipeline:**

In both physical auto-attack and skill damage paths, after calculating damage but before applying to HP:

```js
const kyrieBuff = getActiveBuff(target, 'kyrie_eleison');
if (kyrieBuff && kyrieBuff.barrierHP > 0) {
    if (damage <= kyrieBuff.barrierHP) {
        kyrieBuff.barrierHP -= damage;
        kyrieBuff.hitsRemaining--;
        damage = 0;  // Fully absorbed
    } else {
        damage -= kyrieBuff.barrierHP;
        kyrieBuff.barrierHP = 0;
    }
    if (kyrieBuff.barrierHP <= 0 || kyrieBuff.hitsRemaining <= 0) {
        removeBuff(target, 'kyrie_eleison');
        broadcastToZone(zone, 'skill:buff_removed', { ... reason: 'broken' });
    }
}
```

**Important mechanics:**
- Misses and Lucky Dodges do NOT consume Kyrie hits
- Magic damage is NOT blocked by Kyrie Eleison
- `combat:damage` events that are blocked should show hitType `'blocked'` or 0 damage
- Kyrie is removed by Lex Divina, Dispel, and certain attacks
- Holy Light (Acolyte skill) cancels Kyrie on target even if it misses

---

### 8. MAGNIFICAT (ID 1002) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff (Party-wide) | All sources |
| Max Level | 5 | All sources |
| rAthena ID | 74 (PR_MAGNIFICAT) | rAthena |
| Target | Self + Party members on screen | All sources |
| Range | Screen-wide (party only) | iRO Wiki Classic |
| SP Cost | 40 (all levels) | All sources |
| Cast Time | 4 seconds (variable, reduced by DEX) | All sources |
| After-Cast Delay | 2 seconds | iRO Wiki Classic |
| Prerequisites | None | All sources |
| Effect | Doubles natural HP and SP regeneration rate | iRO Wiki Classic |

**Per-Level Stats:**

| Lv | Duration |
|----|----------|
| 1 | 30s |
| 2 | 45s |
| 3 | 60s |
| 4 | 75s |
| 5 | 90s |

#### Current Definition Status: NEEDS CORRECTION

- `spCost: 40` — **CORRECT**
- `castTime: 4000` — **CORRECT**
- `duration: 30000+i*10000` gives 30, 40, 50, 60, 70 seconds — **WRONG** (should be 30, 45, 60, 75, 90)
- `effectValue: 100` — intended to mean 100% regen bonus (double). OK for handler use.

**Fix:** Duration should be `30000 + i * 15000` giving 30, 45, 60, 75, 90.

#### Implementation Required

**New buff type: `magnificat`**

```js
{
    name: 'magnificat',
    hpRegenMultiplier: 2,   // Double HP regen
    spRegenMultiplier: 2,   // Double SP regen
    duration: duration
}
```

**Integration with regen system:**

In the HP/SP regen tick (currently 6s HP, 8s SP), check for Magnificat buff:

```js
let hpRegenAmount = baseHPRegen;
let spRegenAmount = baseSPRegen;
const magnBuff = getActiveBuff(player, 'magnificat');
if (magnBuff) {
    hpRegenAmount *= magnBuff.hpRegenMultiplier;
    spRegenAmount *= magnBuff.spRegenMultiplier;
}
```

**Party-wide:** Deferred until party system exists. For now, self-only.

---

### 9. GLORIA (ID 1003) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive Buff (Party-wide) | All sources |
| Max Level | 5 | All sources |
| rAthena ID | 75 (PR_GLORIA) | rAthena |
| Target | Self + Party members on screen | All sources |
| Range | Screen-wide (party only) | iRO Wiki Classic |
| SP Cost | 20 (all levels) | All sources |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | 2 seconds | iRO Wiki Classic |
| Prerequisites | Kyrie Eleison Lv4, Magnificat Lv3 | All sources |
| Effect | +30 LUK (flat, all levels, does not scale) | All sources |

**Per-Level Stats:**

| Lv | Duration |
|----|----------|
| 1 | 10s |
| 2 | 15s |
| 3 | 20s |
| 4 | 25s |
| 5 | 30s |

#### Current Definition Status: CORRECT

- `spCost: 20` — **CORRECT**
- `castTime: 0` — **CORRECT**
- `effectValue: 30` — **CORRECT** (+30 LUK)
- `duration: 10000+i*5000` gives 10, 15, 20, 25, 30 — **CORRECT**

#### Implementation Required

**New buff type: `gloria`**

```js
{
    name: 'gloria',
    bonusLUK: 30,
    duration: duration
}
```

**Integration:** `getBuffStatModifiers()` should include `bonusLUK` and add it to effective LUK. This affects critical rate, perfect dodge, and status effect resistance.

**Party-wide:** Deferred until party system. Self-only for now.

---

### 10. RESURRECTION (ID 1004) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive | All sources |
| Max Level | 4 | All sources |
| rAthena ID | 54 (ALL_RESURRECTION) | rAthena |
| Target | Single dead player | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Catalyst | 1 Blue Gemstone (ID 717) | All sources |
| Prerequisites | Heal Lv4 (Aco), Inc SP Recovery Lv4 (rAthena says Aco SP Recovery, some say Priest) | rAthena |

**Per-Level Stats:**

| Lv | SP | Cast Time | HP Restored |
|----|-----|-----------|-------------|
| 1 | 60 | 6s | 10% MaxHP |
| 2 | 60 | 4s | 30% MaxHP |
| 3 | 60 | 2s | 50% MaxHP |
| 4 | 60 | 0s (instant) | 80% MaxHP |

#### Current Definition Status: CORRECT

Manual levels array matches canonical values exactly.

#### Implementation Required

**NEW SYSTEM: Player Resurrection**

This requires the death/respawn system to support third-party resurrection (currently only self-respawn exists).

```js
if (skill.name === 'resurrection') {
    // 1. Find dead player (targetId must be a dead player character)
    const targetPlayer = connectedPlayers.get(targetId);
    if (!targetPlayer || !targetPlayer.isDead) {
        socket.emit('skill:error', { message: 'Target must be a dead player' });
        return;
    }
    // 2. Consume Blue Gemstone (already in SKILL_CATALYSTS)
    // 3. Deduct SP
    // 4. Restore HP to effectVal% of MaxHP
    const restoredHP = Math.floor(targetPlayer.maxHealth * effectVal / 100);
    targetPlayer.isDead = false;
    targetPlayer.health = restoredHP;
    // 5. Broadcast resurrection
    broadcastToZone(zone, 'player:resurrected', {
        characterId: targetId, health: restoredHP, maxHealth: targetPlayer.maxHealth
    });
    // 6. Emit to resurrected player
    targetSocket.emit('combat:health_update', { ... });
    targetSocket.emit('player:respawned', { ... });
}
```

**Death system integration:** Currently `player:request_respawn` is the only way to revive. Resurrection would need to bypass this and directly revive at current position.

---

### 11. MAGNUS EXORCISMUS (ID 1005) — Active, Offensive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Offensive Magic (Ground AoE) | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 79 (PR_MAGNUS) | rAthena |
| Target | Ground 7x7 cross-shaped (350 UE units) | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Element | Holy | All sources |
| Cast Time | 15 seconds (variable, reduced by DEX) | RagnaCloneDocs |
| After-Cast Delay | 4 seconds | iRO Wiki Classic |
| Catalyst | 1 Blue Gemstone (ID 717) | All sources |
| Prerequisites | Turn Undead Lv3, Lex Aeterna Lv1 | All sources |
| Target Restriction | Only damages Undead element AND Demon race enemies | All sources |
| Immunity Period | 3 seconds after being hit (per-monster) | iRO Wiki Classic |

**Per-Level Stats:**

| Lv | SP | Waves | Duration (s) | MATK%/Wave |
|----|-----|-------|-------------|-----------|
| 1 | 40 | 1 | 5 | 100% |
| 2 | 42 | 2 | 6 | 100% |
| 3 | 44 | 3 | 7 | 100% |
| 4 | 46 | 4 | 8 | 100% |
| 5 | 48 | 5 | 9 | 100% |
| 6 | 50 | 6 | 10 | 100% |
| 7 | 52 | 7 | 11 | 100% |
| 8 | 54 | 8 | 12 | 100% |
| 9 | 56 | 9 | 13 | 100% |
| 10 | 58 | 10 | 14 | 100% |

Each wave = 1 hit of 100% MATK (Holy). Waves occur at 3-second intervals due to the immunity mechanic. Maximum 5 waves can hit a single target (since 5 waves x 3s immunity = 15s > 14s max duration, some sources say max 5 full waves at Lv10).

**Bonus damage:** +30% damage vs Undead race, Demon race, Shadow property, and Undead property monsters.

#### Current Definition Status: NEEDS CORRECTION

- `spCost: 40+i*2` gives 40, 42...58 — **CORRECT**
- `castTime: 15000` — **CORRECT**
- `cooldown: 4000` — should be `afterCastDelay: 4000, cooldown: 0`
- `effectValue: 100+i*20` gives 100, 120, 140... — **WRONG** (each wave is 100% MATK; the value 100+i*20 looks like it was trying to scale damage but should just be 100)
- `duration: 5000+i*500` gives 5, 5.5, 6, 6.5... — **WRONG** (should be 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 seconds)

**Fix:** `effectValue: 100` (constant), `duration: 5000 + i * 1000` for correct scaling.

#### Implementation Required

**NEW SYSTEM: Multi-Wave Ground AoE Magic**

This is the most complex Priest skill. Requires a persistent ground effect with periodic wave damage.

```js
// Ground effect:
{
    type: 'magnus_exorcismus',
    x: groundX, y: groundY, z: groundZ,
    radius: 350,           // 7x7 cells cross-shaped
    numWaves: learnedLevel, // 1-10
    waveInterval: 3000,     // Base wave every 3 seconds
    lastWaveTime: Date.now(),
    wavesCompleted: 0,
    matkPercent: 100,       // 100% MATK per wave
    expiresAt: Date.now() + duration,
    casterId: characterId,
    casterStats: getEffectiveStats(player),  // Snapshot at cast time
    zone: zone,
    immuneEnemies: new Map()  // enemyId -> immuneUntil timestamp
}
```

**Tick logic:**
```js
for each magnus ground effect:
    if wavesCompleted >= numWaves or now >= expiresAt:
        removeGroundEffect(effect);
        continue;
    if now >= lastWaveTime + waveInterval:
        // Find Undead element OR Demon race enemies in radius
        for each enemy in radius:
            if enemy.element.type !== 'undead' && enemy.race !== 'demon':
                continue;  // Only hits Undead/Demon
            if immuneEnemies.get(enemy.id) > now:
                continue;  // Still immune from last wave
            // Calculate magical damage: 100% MATK Holy
            const damage = calculateMagicSkillDamage(casterStats, ...);
            // +30% bonus if Undead race, Demon race, Shadow element, or Undead element
            if (['undead', 'demon'].includes(enemy.race) ||
                ['shadow', 'undead'].includes(enemy.element.type)) {
                damage = Math.floor(damage * 1.3);
            }
            enemy.health = Math.max(0, enemy.health - damage);
            immuneEnemies.set(enemy.id, now + 3000);  // 3s immunity
            broadcastToZone(zone, 'skill:effect_damage', { ... });
        wavesCompleted++;
        lastWaveTime = now;
```

---

### 12. TURN UNDEAD (ID 1006) — Active, Offensive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Offensive Magic | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 77 (PR_TURNUNDEAD) | rAthena |
| Target | Single enemy (Undead element only) | All sources |
| Range | 5 cells (~250 UE units) | iRO Wiki Classic |
| Element | Holy (piercing damage) | All sources |
| SP Cost | 20 (all levels) | All sources |
| Cast Time | 1 second (variable) | All sources |
| Cooldown | 500ms | Current definition |
| Prerequisites | Resurrection Lv1, Lex Divina Lv1 | rAthena |

**Instant Kill Chance Formula (Pre-Renewal):**

```
Chance% = (20 * SkillLv + BaseLv + INT + LUK) / 1000 * SkillLv
```

Simplified per-level base chance (ignoring stats):

| Lv | Base % (stats=0) |
|----|-----------------|
| 1 | 2% |
| 2 | 4% |
| 3 | 6% |
| 4 | 8% |
| 5 | 10% |
| 6 | 12% |
| 7 | 14% |
| 8 | 16% |
| 9 | 18% |
| 10 | 20% |

With INT 99 + LUK 30 + BaseLv 99 at Lv10: `(200 + 99 + 99 + 30) / 1000 * 10 = 428/1000 * 10 = 4.28` = ~42.8% instant kill chance.

**Damage on Failure:**

```
Damage = (BaseLv + INT + SkillLv * 10) * 2
```

At BaseLv 99, INT 99, Lv10: `(99 + 99 + 100) * 2 = 596` Holy damage.

Note: This is piercing damage (ignores MDEF).

**Boss monsters:** Can ONLY deal fail-damage to bosses. Instant kill never works on bosses even if they are Undead.

**Non-Undead enemies:** Skill cannot be used. Emit error.

#### Current Definition Status: NEEDS CORRECTION

- `spCost: 20` — **CORRECT**
- `castTime: 1000` — **CORRECT**
- `cooldown: 500` — acceptable
- `effectValue: i*3` gives 0, 3, 6, 9...27 — **NOT USED** for the canonical formula. The handler should calculate from INT/LUK/BaseLv, not from `effectValue`.

**Fix:** `effectValue` doesn't directly map to the formula. Handler should calculate chance and fail-damage from stats directly.

#### Implementation Required

```js
if (skill.name === 'turn_undead') {
    const enemy = enemies.get(targetId);
    if (!enemy || enemy.isDead) { ... return; }
    if (enemy.element?.type !== 'undead') {
        socket.emit('skill:error', { message: 'Target must be Undead element' });
        return;
    }
    // Deduct SP, apply delays
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    const stats = getEffectiveStats(player);
    const baseLv = player.level || 1;
    const intStat = stats.int || 1;
    const lukStat = stats.luk || 1;

    // Instant kill chance (only for non-boss Undead)
    if (!enemy.isBoss && !enemy.modeFlags?.statusImmune) {
        const killChance = (20 * learnedLevel + baseLv + intStat + lukStat) / 1000 * learnedLevel;
        if (Math.random() * 100 < killChance * 100) {
            // Instant kill!
            enemy.health = 0;
            broadcastToZone(zone, 'skill:effect_damage', {
                damage: enemy.maxHealth, hitType: 'magical', element: 'holy', ...
            });
            await processEnemyDeathFromSkill(enemy, player, characterId, io);
            // ...emit skill:used, health_update
            return;
        }
    }
    // Fail damage (piercing, ignores MDEF)
    const failDamage = (baseLv + intStat + learnedLevel * 10) * 2;
    enemy.health = Math.max(0, enemy.health - failDamage);
    broadcastToZone(zone, 'skill:effect_damage', {
        damage: failDamage, hitType: 'magical', element: 'holy', ...
    });
    if (enemy.health <= 0) await processEnemyDeathFromSkill(enemy, player, characterId, io);
    // ...emit
}
```

---

### 13. LEX AETERNA (ID 1007) — Active, Debuff

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Debuff | All sources |
| Max Level | 1 | All sources |
| rAthena ID | 78 (PR_LEXAETERNA) | rAthena |
| Target | Single enemy | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| SP Cost | 10 | All sources |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | 3 seconds (or cooldown 3s) | iRO Wiki Classic |
| Prerequisites | Lex Divina Lv5 | All sources |
| Effect | Next damage source deals DOUBLE damage | All sources |
| Duration | Until consumed (infinite until hit) | iRO Wiki Classic |

#### Current Definition Status: MOSTLY CORRECT

- `spCost: 10` — **CORRECT**
- `castTime: 0` — **CORRECT**
- `cooldown: 3000` — **CORRECT** (3s)
- `effectValue: 200` — **CORRECT** (200% = double damage)

#### Implementation Required

**NEW SYSTEM: One-Hit Debuff (consumable on damage)**

```js
// New buff type: 'lex_aeterna'
{
    name: 'lex_aeterna',
    damageMultiplier: 2,    // Double damage
    consumeOnDamage: true   // Removed after first real damage
}
```

**Integration with ALL damage paths:**

In both physical and magical damage calculations, after computing final damage:

```js
const lexBuff = getActiveBuff(target, 'lex_aeterna');
if (lexBuff) {
    damage = damage * 2;  // Double damage
    removeBuff(target, 'lex_aeterna');
    broadcastToZone(zone, 'skill:buff_removed', { ... });
}
```

**Important:** Lex Aeterna is consumed by the FIRST real damage source of any kind (auto-attack, skill, DoT). It is NOT consumed by 0-damage events, misses, or blocked hits.

**Cannot apply to:** Frozen or Stone Cursed targets (they already have 125% magic damage vulnerability; Lex Aeterna would be too powerful). Server should check: if target has `freeze` or `stone` status, reject with `skill:error`.

---

### 14. LEX DIVINA (ID 1015) — Active, Debuff

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Debuff (Silence) | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 76 (PR_LEXDIVINA) | rAthena |
| Target | Single enemy or ally | All sources |
| Range | 5 cells (~250 UE units) | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| After-Cast Delay | 3 seconds (or cooldown 3s) | iRO Wiki Classic |
| Prerequisites | Ruwach Lv1 (Acolyte) | All sources |
| Special | If target is already silenced, REMOVES silence instead | iRO Wiki Classic |

**Per-Level Stats:**

| Lv | SP | Silence Duration |
|----|-----|-----------------|
| 1 | 20 | 30s |
| 2 | 20 | 35s |
| 3 | 20 | 40s |
| 4 | 20 | 45s |
| 5 | 20 | 50s |
| 6 | 18 | 50s |
| 7 | 16 | 50s |
| 8 | 14 | 50s |
| 9 | 12 | 55s |
| 10 | 10 | 60s |

Note: Duration plateaus at 50s for Lv5-8, then increases again for Lv9-10. SP cost DECREASES at higher levels (20 down to 10).

#### Current Definition Status: NEEDS CORRECTION

- `spCost: i < 5 ? 20-i*2 : 20-i*2` — this formula gives 20, 18, 16, 14, 12, 10, 8, 6, 4, 2 — **WRONG** for Lv1-5 (should be 20 for all). The conditional doesn't actually branch differently.
- `duration: 30000+i*3000` gives 30, 33, 36, 39, 42, 45, 48, 51, 54, 57 — **WRONG** (should be 30, 35, 40, 45, 50, 50, 50, 50, 55, 60)
- `cooldown: 3000` — **CORRECT**

**Fix:** Use manual SP array: `[20, 20, 20, 20, 20, 18, 16, 14, 12, 10]`
**Fix:** Use manual duration array: `[30000, 35000, 40000, 45000, 50000, 50000, 50000, 50000, 55000, 60000]`

#### Implementation Required

```js
if (skill.name === 'lex_divina') {
    // Check if target is already silenced -> remove silence
    const targetEntity = isEnemy ? enemies.get(targetId) : connectedPlayers.get(targetId);
    if (targetEntity?.activeStatusEffects?.has('silence')) {
        removeStatusEffect(targetEntity, 'silence');
        broadcastToZone(zone, 'status:removed', {
            targetId, isEnemy, statusType: 'silence', reason: 'cured'
        });
    } else {
        // Apply silence (100% success rate, no resistance check in pre-renewal)
        applyStatusEffect(targetEntity, 'silence', {
            duration: duration,
            sourceId: characterId,
            sourceLevel: player.level
        });
        broadcastToZone(zone, 'status:applied', {
            targetId, isEnemy, statusType: 'silence', duration
        });
    }
    // SP, delays, used emit
}
```

---

### 15. STATUS RECOVERY (ID 1014) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive | All sources |
| Max Level | 1 | All sources |
| rAthena ID | 72 (PR_STRECOVERY) | rAthena |
| Target | Self or single ally | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| SP Cost | 5 | All sources |
| Cast Time | 2 seconds (some sources say 0) | RagnaCloneDocs says 2s |
| After-Cast Delay | None | iRO Wiki Classic |
| Cooldown | 2 seconds | Current definition |
| Prerequisites | None | All sources |
| Cures | Frozen, Stone (all stages), Stun | All sources |

#### Current Definition Status: ACCEPTABLE

- `spCost: 5` — **CORRECT**
- `castTime: 0` — **DEBATABLE** (RagnaCloneDocs says 2s, some sources say 0). Keep as 0 for usability.
- `cooldown: 2000` — acceptable anti-spam measure

#### Implementation Required

```js
if (skill.name === 'status_recovery') {
    const target = isEnemy ? null : connectedPlayers.get(targetId || characterId);
    if (!target) { socket.emit('skill:error', ...); return; }
    // Deduct SP, apply delays
    const cured = cleanseStatusEffects(target, ['freeze', 'stone', 'stun']);
    for (const status of cured) {
        broadcastToZone(zone, 'status:removed', {
            targetId: targetId || characterId, isEnemy: false,
            statusType: status, reason: 'cured'
        });
    }
    // Emit skill:used, health_update
}
```

---

### 16. SLOW POISON (ID 1013) — Active, Supportive

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Supportive | All sources |
| Max Level | 4 | All sources |
| rAthena ID | 71 (PR_SLOWPOISON) | rAthena |
| Target | Self or single ally | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Cast Time | None | iRO Wiki Classic |
| Prerequisites | None | All sources |
| Effect | Temporarily halts poison HP drain (does NOT cure poison) | All sources |

**Per-Level Stats:**

| Lv | SP | Duration |
|----|-----|----------|
| 1 | 6 | 10s |
| 2 | 8 | 20s |
| 3 | 10 | 30s |
| 4 | 12 | 40s |

#### Current Definition Status: CORRECT

`spCost: 6+i*2` gives 6, 8, 10, 12. `duration: 10000+i*10000` gives 10, 20, 30, 40 seconds.

#### Implementation Required

**New buff type: `slow_poison`**

```js
{
    name: 'slow_poison',
    pausePoisonDrain: true,
    duration: duration
}
```

**Integration with status tick:** In the poison HP drain tick, check:
```js
if (entity.activeStatusEffects?.has('poison')) {
    const slowBuff = getActiveBuff(entity, 'slow_poison');
    if (!slowBuff) {
        // Apply poison HP drain as normal
    }
    // Else: skip drain, poison still active but paused
}
```

---

### 17. INCREASE SP RECOVERY (Priest) (ID 1016) — Passive

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| rAthena ID | N/A (Priest version) | Project-specific |
| Effect | +3 SP regen per level (same as Mage version, ID 204) | All sources |
| Prerequisites | None | All sources |

#### Current Definition Status: CORRECT

`effectValue: (i+1)*3` gives +3 to +30 SP regen.

#### Implementation Required

Add to `getPassiveSkillBonuses()`:

```js
const priestSpRecLv = learned[1016] || 0;
if (priestSpRecLv > 0) {
    bonuses.spRegenBonus += priestSpRecLv * 3;
}
```

Note: This stacks with the Mage version (ID 204) if somehow both are learned (shouldn't happen due to class progression, but defensive coding).

---

### 18. SAFETY WALL (Priest) (ID 1017) — Active, Ground

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Ground (persistent) | All sources |
| Max Level | 10 | All sources |
| rAthena ID | 12 (MG_SAFETYWALL) — shared with Mage | rAthena |
| Target | Ground 1x1 cell | All sources |
| Range | 9 cells (~450 UE units) | iRO Wiki Classic |
| Catalyst | 1 Blue Gemstone (ID 717) | All sources |
| Prerequisites | Increase SP Recovery (Priest) Lv3 | Current definition |
| Effect | Blocks melee attacks (ranged and magic pass through) | All sources |

**Per-Level Stats:**

| Lv | SP | Cast Time (ms) | Hits Blocked | Duration (s) | Durability |
|----|----|---------------|-------------|-------------|------------|
| 1 | 30 | 4000 | 2 | 5 | 300 |
| 2 | 30 | 3500 | 3 | 10 | 600 |
| 3 | 30 | 3500 | 4 | 15 | 900 |
| 4 | 35 | 2500 | 5 | 20 | 1200 |
| 5 | 35 | 2000 | 6 | 25 | 1500 |
| 6 | 35 | 1500 | 7 | 30 | 1800 |
| 7 | 40 | 1000 | 8 | 35 | 2100 |
| 8 | 40 | 1000 | 9 | 40 | 2400 |
| 9 | 40 | 1000 | 10 | 45 | 2700 |
| 10 | 40 | 1000 | 11 | 50 | 3000 |

Durability = MaxSP-dependent bonus in pre-renewal. The base durability is `300 * level`.

#### Current Definition Status: CORRECT (complex manual arrays)

The definition uses manual arrays for SP, castTime, and effectValue. These match canonical data.

#### Implementation Required

**Ground effect type: `safety_wall`**

```js
{
    type: 'safety_wall',
    x: groundX, y: groundY, z: groundZ,
    radius: 50,              // 1x1 cell
    hitsRemaining: effectVal, // 2-11
    durability: effectVal * 300, // 300 per level
    expiresAt: Date.now() + duration,
    zone: zone
}
```

**Integration with melee damage:**

In auto-attack and melee skill paths:

```js
const sw = getGroundEffectAtPosition(target.x, target.y, 'safety_wall');
if (sw) {
    sw.hitsRemaining--;
    if (sw.hitsRemaining <= 0) {
        removeGroundEffect(sw);
        broadcastToZone(zone, 'skill:ground_effect_removed', { type: 'safety_wall', ... });
    }
    // Block damage entirely
    damage = 0;
    hitType = 'blocked';
}
```

**Overlap rule:** Cannot overlap with Pneuma. Cannot stack multiple Safety Walls on same cell.

---

### 19. REDEMPTIO (ID 1018) — Active, Quest Skill

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self (affects party) | All sources |
| Max Level | 1 | All sources |
| rAthena ID | 1014 (PR_REDEMPTIO) | rAthena |
| Target | Self-centered 15x15 AoE (party only) | All sources |
| SP Cost | 400 | All sources |
| Cast Time | 4 seconds | All sources |
| Effect | Resurrect all dead party members in range at 50% HP | All sources |
| Caster Penalty | HP and SP drop to 1 | All sources |
| EXP Cost | 1% of required base EXP to next level, reduced by 0.2% per member resurrected | iRO Wiki Classic |
| Quest Skill | Must be learned via quest (not skill points) | All sources |

#### Current Definition Status: CORRECT

`spCost: 400`, `castTime: 4000`, `effectValue: 50` (50% HP restore), `questSkill: true`.

#### Implementation Required

**DEFERRED** — Requires party system. This is a mass resurrection that affects only party members. Without parties, this skill has no targets.

When party system is implemented:
1. Find all dead party members within 750 UE units (15x15 cells)
2. Resurrect each at 50% MaxHP
3. Set caster HP=1, SP=1
4. Deduct `floor(requiredBaseEXP * 0.01 * (1 - 0.002 * numResurrected))` from caster's base EXP

---

## New Systems Required

| System | Skills Affected | Priority | Effort | Description |
|--------|----------------|----------|--------|-------------|
| **Damage Absorb Barrier** | Kyrie Eleison | HIGH | Medium | Buff with HP-based shield + hit counter, checked in all damage paths |
| **Ground Heal Zone** | Sanctuary | HIGH | Large | Persistent ground effect with periodic heal ticks + Undead damage |
| **Multi-Wave Ground AoE** | Magnus Exorcismus | MEDIUM | Large | Persistent ground effect with wave damage, per-enemy immunity timer |
| **One-Hit Consumable Debuff** | Lex Aeterna | HIGH | Small | Buff that doubles damage then removes itself |
| **Element Endow Buff** | Aspersio | HIGH | Small | Override weapon element via buff, integration with attackerInfo |
| **Armor Element Endow** | B.S. Sacramenti | LOW | Small | Override armor element via buff (needs 2 Aco check, deferred) |
| **Cast Time Reduction Buff** | Suffragium | MEDIUM | Small | Consumable buff that reduces next spell's cast time |
| **Regen Multiplier Buff** | Magnificat | MEDIUM | Small | Double HP/SP regen in regen tick loop |
| **LUK Bonus Buff** | Gloria | MEDIUM | Small | +30 LUK in stat calculations |
| **Player Resurrection** | Resurrection | LOW | Medium | Third-party revive system (currently only self-respawn) |
| **Mace Mastery Passive** | Mace Mastery | HIGH | Trivial | Add to getPassiveSkillBonuses() |
| **Flat ATK Buff** | Impositio Manus | HIGH | Small | Flat ATK bonus in buff system |
| **Silence via Skill** | Lex Divina | MEDIUM | Small | Apply/remove silence status, dual-purpose logic |
| **Status Cleanse** | Status Recovery | MEDIUM | Trivial | cleanseStatusEffects for freeze/stone/stun |
| **Poison Drain Pause** | Slow Poison | LOW | Trivial | Buff flag to skip poison HP drain |
| **SP Regen Passive** | Inc SP Recovery (Priest) | HIGH | Trivial | Add to getPassiveSkillBonuses() |
| **Melee Hit Block Ground** | Safety Wall (Priest) | MEDIUM | Medium | Ground effect that blocks melee, counted hits |
| **Instant Kill Mechanic** | Turn Undead | MEDIUM | Medium | Probability-based instant death for Undead |
| **Mass Resurrect** | Redemptio | DEFERRED | Large | Requires party system |
| **Catalyst Entries** | Safety Wall, BSS | HIGH | Trivial | Add to SKILL_CATALYSTS |

---

## Skill Data Corrections

The following corrections are needed in `server/src/ro_skill_data_2nd.js`:

### Sanctuary (ID 1000)
```
FIX effectValue: Use manual array [100, 200, 300, 400, 500, 600, 777, 777, 777, 777]
FIX duration: Change from 4000+i*1000 to manual array [4000, 7000, 10000, 13000, 16000, 19000, 22000, 25000, 28000, 31000]
ADD afterCastDelay: 2000
```

### Kyrie Eleison (ID 1001)
```
FIX spCost: Change from 20+i*2 to manual array [20, 20, 20, 25, 25, 25, 30, 30, 30, 35]
ADD afterCastDelay: 2000
ADD maxHits field or calculate in handler: floor(level/2) + 5
```

### Magnificat (ID 1002)
```
FIX duration: Change from 30000+i*10000 to 30000+i*15000 (gives 30, 45, 60, 75, 90)
ADD afterCastDelay: 2000
```

### Gloria (ID 1003)
```
ADD afterCastDelay: 2000
```

### Magnus Exorcismus (ID 1005)
```
FIX effectValue: Change from 100+i*20 to constant 100
FIX duration: Change from 5000+i*500 to 5000+i*1000 (gives 5, 6, 7...14 seconds)
CHANGE cooldown: 4000 -> afterCastDelay: 4000, cooldown: 0
FIX prerequisites: Should be [Turn Undead 3, Lex Aeterna 1] plus Safety Wall 1
   Current: [Teleport 1, Sanctuary 1] — WRONG
   Correct: [{ skillId: 1006, level: 3 }, { skillId: 1007, level: 1 }]
   Note: Safety Wall prereq is complex (Mage or Priest Safety Wall)
```

### Turn Undead (ID 1006)
```
FIX prerequisites: Should include Lex Divina Lv1
   Current: [Resurrection 1] — INCOMPLETE
   Correct: [{ skillId: 1004, level: 1 }, { skillId: 1015, level: 1 }]
```

### Lex Aeterna (ID 1007)
```
FIX prerequisites: Should require Lex Divina Lv5
   Current: [] — MISSING
   Correct: [{ skillId: 1015, level: 5 }]
```

### Lex Divina (ID 1015)
```
FIX spCost: Change from 20-i*2 to manual array [20, 20, 20, 20, 20, 18, 16, 14, 12, 10]
FIX duration: Change from 30000+i*3000 to manual array [30000, 35000, 40000, 45000, 50000, 50000, 50000, 50000, 55000, 60000]
```

### Impositio Manus (ID 1009)
```
No changes needed — definition is correct.
```

### Suffragium (ID 1010)
```
No changes needed — manual levels are correct.
```

### Aspersio (ID 1011)
```
Verify SP formula: 14+i*4 gives 14, 18, 22, 26, 30 — matches canonical. OK.
```

---

## Implementation Priority

### Phase 1: Passives and Simple Buffs (HIGH priority, small effort)

1. **Mace Mastery passive** — add to `getPassiveSkillBonuses()` (+3 ATK/lv with maces)
2. **Increase SP Recovery (Priest) passive** — add to `getPassiveSkillBonuses()` (+3 SP regen/lv)
3. **Impositio Manus handler** — flat ATK buff, standard buff template
4. **Gloria handler** — +30 LUK buff, standard buff template
5. **Skill data corrections** — fix all definitions listed above

### Phase 2: Core Support Skills (HIGH priority, medium effort)

6. **Kyrie Eleison handler + barrier system** — MaxHP% damage absorb shield
7. **Lex Aeterna handler** — one-hit double damage debuff
8. **Aspersio handler** — Holy weapon endow buff
9. **Status Recovery handler** — cleanse freeze/stone/stun
10. **Lex Divina handler** — silence / cure silence dual-purpose

### Phase 3: Ground Effects (MEDIUM priority, large effort)

11. **Sanctuary handler + ground heal zone** — periodic heal ticks, Undead damage
12. **Safety Wall (Priest) handler** — ground melee block with hit counter
13. **Magnificat handler** — regen multiplier buff (simple, but needs regen integration)
14. **Suffragium handler** — cast time reduction buff (needs cast system integration)

### Phase 4: Offensive Skills (MEDIUM priority, large effort)

15. **Turn Undead handler** — instant kill chance + fail damage
16. **Magnus Exorcismus handler** — multi-wave ground AoE (most complex Priest skill)

### Phase 5: Utility and Edge Cases (LOW priority)

17. **Slow Poison handler** — pause poison drain buff
18. **B.S. Sacramenti handler** — Holy armor endow (simplified, no 2-Aco check)
19. **Resurrection handler** — player revive system

### Phase 6: Deferred (requires other systems)

20. **Redemptio** — requires party system
21. **Party-wide buffs** (Magnificat, Gloria, Angelus expansion) — requires party system
22. **B.S. Sacramenti 2-Acolyte check** — requires party/proximity system
23. **Sanctuary target limit** — requires proper target count per tick

---

## Integration Points

### Buff System (`ro_buff_system.js` / `index.js`)

New buff types to register:

| Buff Name | Key Fields | Duration | Stackable? |
|-----------|-----------|----------|------------|
| `impositio_manus` | `bonusATK` | 60s | No (overwrite) |
| `suffragium` | `castTimeReduction`, `consumeOnCast` | 10-30s | No (overwrite) |
| `aspersio` | `weaponElement: 'holy'` | 60-180s | No (overwrite) |
| `bs_sacramenti` | `armorElement: {type:'holy',level:1}` | 40-200s | No (overwrite) |
| `kyrie_eleison` | `barrierHP`, `maxHits`, `hitsRemaining` | 120s | No (overwrite) |
| `magnificat` | `hpRegenMultiplier`, `spRegenMultiplier` | 30-90s | No (overwrite) |
| `gloria` | `bonusLUK` | 10-30s | No (overwrite) |
| `slow_poison` | `pausePoisonDrain` | 10-40s | No (overwrite) |
| `lex_aeterna` | `damageMultiplier`, `consumeOnDamage` | Infinite (until hit) | No (overwrite) |

### `getBuffStatModifiers()` Extensions

Add handling for:
- `bonusATK` (from `impositio_manus`) -> add to effective ATK
- `bonusLUK` (from `gloria`) -> add to effective LUK
- `weaponElement` (from `aspersio`) -> override weapon element
- `armorElement` (from `bs_sacramenti`) -> override armor element
- `hpRegenMultiplier` / `spRegenMultiplier` (from `magnificat`) -> multiply regen values
- `castTimeReduction` (from `suffragium`) -> reduce next cast time
- `pausePoisonDrain` (from `slow_poison`) -> skip poison drain tick

### Damage Pipeline Integration

**Kyrie Eleison barrier check:** After damage calculation, before HP reduction, in:
- `processEnemyAutoAttack()` — player auto-attack
- `processPlayerAutoAttack()` — PvP auto-attack
- Physical skill execution paths

**Lex Aeterna double damage:** After final damage calculation, before HP reduction, in:
- ALL damage paths (physical, magical, auto-attack, skill)

**Safety Wall melee block:** Before damage calculation, in:
- Auto-attack paths (melee only)
- Melee skill execution paths
- NOT in ranged or magical damage paths

### Ground Effect System

New ground effect types needed:
- `sanctuary` — periodic heal + Undead damage
- `magnus_exorcismus` — periodic wave damage
- `safety_wall` — melee hit block

These integrate with the existing `createGroundEffect()` / `getGroundEffectsAtPosition()` system used by Fire Wall and Pneuma.

### Passive Skill System

Add to `getPassiveSkillBonuses()`:
- Mace Mastery (ID 1008): `bonusATK += level * 3` when `weaponType === 'mace'`
- Inc SP Recovery (ID 1016): `spRegenBonus += level * 3`

### Cast Time System

Suffragium integration in the cast time calculation block of `skill:use`:
```js
// After DEX-based cast time reduction
const suffBuff = getActiveBuff(player, 'suffragium');
if (suffBuff && actualCastTime > 0) {
    actualCastTime = Math.floor(actualCastTime * (100 - suffBuff.castTimeReduction) / 100);
    removeBuff(player, 'suffragium');
}
```

### Client-Side VFX

Each Priest skill needs a VFX config in `SkillVFXData.cpp`:

| Skill ID | Template | Color | Notes |
|----------|----------|-------|-------|
| 1000 | GroundPersistent | Gold/White | Sanctuary healing circle |
| 1001 | SelfBuff / HealFlash | White/Blue | Kyrie barrier aura |
| 1002 | SelfBuff | Gold | Magnificat prayer aura |
| 1003 | SelfBuff | Gold/Yellow | Gloria light burst |
| 1004 | HealFlash | White/Gold | Resurrection pillar of light |
| 1005 | GroundPersistent | Bright Holy/Cross | Magnus cross on ground |
| 1006 | Projectile / AoEImpact | Holy/White | Turn Undead holy strike |
| 1007 | TargetDebuff | Purple/Dark | Lex Aeterna curse mark |
| 1009 | SelfBuff / HealFlash | Yellow | Impositio hand blessing |
| 1010 | HealFlash | Blue/White | Suffragium cast glow |
| 1011 | SelfBuff | Holy/Gold | Aspersio holy water splash |
| 1012 | GroundPersistent | Holy/Gold | BSS ground blessing |
| 1013 | HealFlash | Green | Slow Poison cure aura |
| 1014 | HealFlash | White/Blue | Status Recovery cleanse |
| 1015 | TargetDebuff | Dark/Purple | Lex Divina silence mark |
| 1017 | GroundPersistent | White/Translucent | Safety Wall shield dome |
| 1018 | SelfBuff | Gold/Bright | Redemptio sacrifice light |

---

## High Priest Preview (Future Phase)

The High Priest is the transcendent version of Priest, inheriting all Acolyte + Priest skills and adding:

### Meditatio (Passive, 10 levels)
- +2% Heal effectiveness per level (up to +20%)
- +1% MaxSP per level (up to +10%)
- +3% SP regen per level (up to +30%)

### Assumptio (Supportive, 5 levels)
- Halves all incoming damage (PvM), reduces by 1/3 (PvP)
- Duration: 20-100s, SP: 20-60, Cast: 1-3s
- Disabled in WoE

### Basilica (Ground, 5 levels)
- 5x5 area where no attacks can enter or leave
- Caster must remain stationary
- Requires 4 catalysts (Blue Gem + Yellow Gem + Holy Water + Red Gem)

### Mana Recharge / Spiritual Thrift (Passive, 5 levels)
- 4-20% SP cost reduction for all skills

These are documented in `RagnaCloneDocs/03_Skills_Complete.md` at line 3715+ and will be implemented when transcendent classes are added.

---

## Priest-Specific Constants

```js
// Sanctuary heal caps
const SANCTUARY_HEAL_PER_TICK = [100, 200, 300, 400, 500, 600, 777, 777, 777, 777];
const SANCTUARY_TARGET_LIMIT = [4, 5, 6, 7, 8, 9, 10, 11, 12, 13];

// Kyrie Eleison barriers
const KYRIE_SP_COST = [20, 20, 20, 25, 25, 25, 30, 30, 30, 35];
const KYRIE_MAX_HITS = [5, 6, 6, 7, 7, 8, 8, 9, 9, 10];

// Lex Divina SP and duration
const LEX_DIVINA_SP = [20, 20, 20, 20, 20, 18, 16, 14, 12, 10];
const LEX_DIVINA_DURATION = [30000, 35000, 40000, 45000, 50000, 50000, 50000, 50000, 55000, 60000];

// Safety Wall (pre-renewal)
const SAFETY_WALL_SP = [30, 30, 30, 35, 35, 35, 40, 40, 40, 40];
const SAFETY_WALL_CAST = [4000, 3500, 3500, 2500, 2000, 1500, 1000, 1000, 1000, 1000];
const SAFETY_WALL_HITS = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
const SAFETY_WALL_DURABILITY_PER_LV = 300;  // 300 * level

// Turn Undead formulas
// Kill chance: (20 * SkillLv + BaseLv + INT + LUK) / 1000 * SkillLv
// Fail damage: (BaseLv + INT + SkillLv * 10) * 2 (piercing, ignores MDEF)

// Magnus Exorcismus
// 100% MATK Holy per wave, 3-second immunity between hits
// +30% bonus damage vs Undead race, Demon race, Shadow element, Undead element
```

---

## Sources

- [Priest - iRO Wiki Classic](https://irowiki.org/classic/Priest)
- [Priest - iRO Wiki](https://irowiki.org/wiki/Priest)
- [Priest Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=8)
- [Priest Skill Simulator - RateMyServer](https://ratemyserver.net/skill_sim.php?jid=8)
- [Magnus Exorcismus - iRO Wiki](https://irowiki.org/wiki/Magnus_Exorcismus)
- [Magnus Exorcismus - iRO Wiki Classic](https://irowiki.org/classic/Magnus_Exorcismus)
- [Magnus Exorcismus - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=79)
- [Turn Undead - iRO Wiki](https://irowiki.org/wiki/Turn_Undead)
- [Turn Undead - iRO Wiki Classic](https://irowiki.org/classic/Turn_Undead)
- [Sanctuary - iRO Wiki Classic](https://irowiki.org/classic/Sanctuary)
- [Sanctuary - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=70)
- [Kyrie Eleison - iRO Wiki Classic](https://irowiki.org/classic/Kyrie_Eleison)
- [Kyrie Eleison - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=73)
- [Safety Wall - iRO Wiki Classic](https://irowiki.org/classic/Safety_Wall)
- [Lex Aeterna - iRO Wiki Classic](https://irowiki.org/classic/Lex_Aeterna)
- [Lex Divina - iRO Wiki Classic](https://irowiki.org/classic/Lex_Divina)
- [Lex Divina - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=76)
- [Impositio Manus - iRO Wiki Classic](https://irowiki.org/classic/Impositio_Manus)
- [Suffragium - iRO Wiki Classic](https://irowiki.org/classic/Suffragium)
- [Aspersio - iRO Wiki Classic](https://irowiki.org/classic/Aspersio)
- [Magnificat - iRO Wiki Classic](https://irowiki.org/classic/Magnificat)
- [Gloria - iRO Wiki Classic](https://irowiki.org/classic/Gloria)
- [Resurrection - iRO Wiki](https://irowiki.org/wiki/Resurrection)
- [Redemptio - iRO Wiki Classic](https://irowiki.org/classic/Redemptio)
- [Mace Mastery - iRO Wiki](https://irowiki.org/wiki/Mace_Mastery)
- [B.S. Sacramenti - iRO Wiki](https://irowiki.org/wiki/B.S_Sacramenti)
- [High Priest - iRO Wiki Classic](https://irowiki.org/classic/High_Priest)
- [rAthena pre-re/skill_tree.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_tree.yml)
- [rAthena pre-re/skill_db.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [RagnaCloneDocs/03_Skills_Complete.md](../../../RagnaCloneDocs/03_Skills_Complete.md) (project-internal reference)
- [Acolyte Skills Audit](./Acolyte_Skills_Audit_And_Fix_Plan.md) (project-internal, parent class)
