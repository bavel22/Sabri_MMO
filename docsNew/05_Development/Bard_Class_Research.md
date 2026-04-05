# Bard Class — Pre-Renewal Implementation Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Bard_Skills_Audit](Bard_Skills_Comprehensive_Audit.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Research Date:** 2026-03-15
**Sources:** iRO Wiki (classic + renewal), RateMyServer Skill DB, rAthena pre-renewal DB (pre.pservero.com), rAthena GitHub (issues #1808, status.cpp, skill.cpp, skill_db.yml), GameFAQs Bard guides, rAthena xmas_present2008 disassembly
**Scope:** Full Bard class implementation plan including skill audit, new systems, and integration points
**Cross-References:** `Hunter_Bard_Dancer_Skills_Research.md` (detailed skill tables), `Dancer_Class_Research.md`, `Archer_Skills_Audit_And_Fix_Plan.md` (parent class)

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Performance System Architecture](#2-performance-system-architecture)
3. [Ensemble System Architecture](#3-ensemble-system-architecture)
4. [Complete Skill Reference](#4-complete-skill-reference)
5. [Skill Tree and Prerequisites](#5-skill-tree-and-prerequisites)
6. [Existing Code Audit](#6-existing-code-audit)
7. [New Systems Required](#7-new-systems-required)
8. [Implementation Priority](#8-implementation-priority)
9. [Integration Points](#9-integration-points)
10. [Data Tables and Constants](#10-data-tables-and-constants)

---

## 1. Class Overview

### Identity

| Property | Value |
|----------|-------|
| **Base Class** | Archer |
| **Gender** | Male only (Dancer is the female counterpart) |
| **Weapon** | Instrument (special weapon class; replaces bow for performance skills) |
| **Also equips** | Dagger (for canceling songs, general combat) |
| **Transcendent** | Clown (Minstrel) |
| **Job Change NPC** | Snowman (Lutie Toy Factory area) |
| **Job Level Cap** | 50 (standard 2nd class) |
| **Skill Points** | 49 (1 per job level, levels 1-49) |
| **HP/SP Class** | Same as Archer base coefficients |
| **rAthena Job ID** | 19 |

### Class Progression Chain

```
Novice -> Archer -> Bard -> Clown (Minstrel) -> Minstrel (Trans)
```

In `CLASS_PROGRESSION`:
```js
'bard': ['novice', 'archer', 'bard'],
'clown': ['novice', 'archer', 'bard', 'clown'],    // future transcendent
'minstrel': ['novice', 'archer', 'bard', 'clown'],  // alias
```

### Weapon Types

| Weapon Type | Can Equip | Used For |
|-------------|-----------|----------|
| Instrument | Yes (primary) | Performance skills, Musical Strike, auto-attack |
| Bow | Yes (inherited from Archer) | Double Strafe, Arrow Shower, all Archer skills |
| Dagger | Yes | General combat, song cancellation trick |
| Arrow (ammo) | Yes (required for Musical Strike + Archer skills) | Consumed per attack/skill |

### Key Design Philosophy

Bards are **support-oriented** 2nd-class characters. Their primary role is buffing party members through **performance songs** (AoE passive auras). While they can deal damage with Musical Strike and auto-attacks, their value comes from songs like Poem of Bragi (cast time/delay reduction) and Assassin Cross of Sunset (ASPD boost). The Bard/Dancer pair system creates ensemble skills with powerful combined effects.

---

## 2. Performance System Architecture

### How Songs Work (Pre-Renewal)

Songs are ground-based AoE buff effects centered on the caster. When a Bard activates a song:

1. **Activation:** SP is consumed. A 7x7 cell ground effect spawns centered on the Bard.
2. **Performance State:** The Bard enters "performing" state. Movement speed is heavily restricted (reduced to ~50% with Music Lessons 10, slower without it). The Bard cannot use other songs or ensembles while performing.
3. **AoE Effect:** All party members (and sometimes enemies, depending on the song) within the 7x7 area receive the buff/debuff effect. The effect persists as long as they remain in the AoE.
4. **SP Drain:** The Bard loses 1 SP every N seconds (varies per song) while performing. When SP reaches 0, the song automatically ends.
5. **Duration:** Pre-renewal songs have a "stay duration" (how long the ground effect persists) and an "effect duration" (how long the buff lingers after leaving the AoE). Typical stay = 60-180s, linger = 20s.
6. **Cancellation:** The song can be canceled by using Adaptation to Circumstances (1 SP), or by switching to a dagger weapon (instant cancel, popular technique). Using another song also cancels the current one.
7. **Musical Strike:** Can be used DURING performance without canceling it.
8. **Movement During Performance:** Without Music Lessons, the Bard cannot move at all while performing. With Music Lessons, movement is partially restored (up to 50% normal speed at Lv10).

### Performance State Properties

```js
// Server-side player state during performance
player.performanceState = {
    active: true,                    // Is performing right now
    skillId: 1501,                   // Which performance skill (Bragi, Whistle, etc.)
    skillLevel: 10,                  // Level of the skill
    startedAt: Date.now(),           // When performance started
    expiresAt: Date.now() + 180000,  // Stay duration limit
    lastSpDrainAt: Date.now(),       // Last SP drain tick
    spDrainInterval: 5000,           // 1 SP every N ms
    aoeRadius: 175,                  // 7x7 cells = 3.5 cell radius = 175 UE units
    centerX: player.x,              // Center position (updates if Bard moves)
    centerY: player.y,
    musicLessonsLv: 10,             // Cached for effect calculations
    effectValues: { ... },           // Pre-calculated song effects (FLEE, ASPD%, etc.)
    lastPerformanceSkillId: 1501,    // For Encore functionality
};
```

### Performance Song AoE

| Context | AoE Size | Radius (UE units) |
|---------|----------|-------------------|
| Solo Song (pre-renewal) | 7x7 cells | 175 (3.5 cells * 50 UE/cell) |
| Ensemble (pre-renewal) | 9x9 cells | 225 (4.5 cells * 50 UE/cell) |
| Solo Song (renewal) | 31x31 cells | 775 |

**Our implementation uses pre-renewal: 7x7 for solos, 9x9 for ensembles.**

### Song Effect Linger

When a player leaves the song AoE, the buff effect lingers for approximately 20 seconds before fading. This prevents the buff from flickering on/off when players are near the edge of the AoE.

Implementation approach: Apply the song buff as a standard buff with 20s duration that refreshes every tick while the player is in the AoE. When they leave, the 20s timer counts down naturally.

### Movement Speed Reduction During Performance

Without Music Lessons, the Bard cannot move while performing. With Music Lessons, movement speed is partially restored:

```
Move Speed During Performance = Base_Speed * (MusicLessons_Lv * 0.05)
```

At Music Lessons 10: 50% normal speed. At Music Lessons 0: 0% (cannot move).

### SP Drain Intervals (Per Song)

| Song | SP Drain Interval | Source |
|------|-------------------|--------|
| A Whistle | 1 SP / 5 seconds | rAthena pre-re |
| Assassin Cross of Sunset | 1 SP / 3 seconds | rAthena pre-re, GameFAQs |
| A Poem of Bragi | 1 SP / 5 seconds | rAthena pre-re |
| Apple of Idun | 1 SP / 6 seconds | rAthena pre-re |
| Dissonance | 1 SP / 3 seconds | rAthena pre-re |

### Song Restrictions

- Only 1 performance can be active at a time (solo OR ensemble, not both).
- Songs do not stack with the same song from another Bard. The higher-level one takes precedence.
- Songs cannot be used while under certain CC effects (frozen, stoned, stunned, silenced).
- Instrument weapon MUST be equipped to start a performance.
- Musical Strike can be used during performance without canceling it.
- Frost Joker can be used during performance without canceling it.
- Other skills (Bash, bolts, etc.) cannot be used during performance.

---

## 3. Ensemble System Architecture

### How Ensembles Work (Pre-Renewal)

Ensemble skills require **both a Bard and a Dancer in the same party, standing adjacent to each other** (within approximately 1-2 cells).

1. **Activation:** Either the Bard or the Dancer can initiate the ensemble. Both must have the skill learned. The resulting skill level = `floor((BardSkillLv + DancerSkillLv) / 2)`.
2. **Both Enter Performance State:** Both the Bard and Dancer enter "performing" state simultaneously. Neither can move, use other songs/dances, or perform other actions while the ensemble is active.
3. **AoE Effect:** A 9x9 cell ground effect spawns centered between the two performers. All players/enemies in the area receive the ensemble effect.
4. **SP Drain:** Both performers lose 1 SP every N seconds.
5. **Duration:** Typically 60 seconds. Ends early if either performer runs out of SP, moves too far apart, or cancels.
6. **Cancellation:** Either performer using Adaptation cancels the ensemble for both. If the performers are separated beyond 1-2 cells, the ensemble breaks.

### Ensemble Prerequisites

Each ensemble skill has different prerequisites for the Bard and Dancer sides:

| Ensemble Skill | Bard Prerequisite | Dancer Prerequisite |
|---------------|-------------------|---------------------|
| Lullaby | A Whistle Lv 10 | Humming Lv 10 |
| Mr. Kim A Rich Man | Invulnerable Siegfried Lv 3 | Invulnerable Siegfried Lv 3 |
| Eternal Chaos | Loki's Veil Lv 1 | Loki's Veil Lv 1 |
| A Drum on the Battlefield | Apple of Idun Lv 10 | Service for You Lv 10 |
| The Ring of Nibelungen | Drum on Battlefield Lv 3 | Drum on Battlefield Lv 3 |
| Loki's Veil | Assassin Cross Lv 10 | Please Don't Forget Me Lv 10 |
| Into the Abyss | Lullaby Lv 1 | Lullaby Lv 1 |
| Invulnerable Siegfried | Poem of Bragi Lv 10 | Fortune's Kiss Lv 10 |

### Implementation Complexity

Ensembles require:
1. Party system (to check if Bard+Dancer are in same party)
2. Proximity detection (to check if they are adjacent)
3. Synchronized state management (both enter/exit performance together)
4. Averaged skill level calculation
5. Shared ground AoE centered between both performers

**Recommendation:** Defer ensemble implementation until the party system is built. Solo songs can be implemented independently.

---

## 4. Complete Skill Reference

### Notation

- rA ID = official rAthena skill ID (used in rAthena database)
- Project ID = our internal ID from `ro_skill_data_2nd.js`
- All formulas are **pre-renewal** unless noted

---

### 4.1 Music Lessons (Passive)

| Field | Value |
|-------|-------|
| **rA ID** | 315 (BA_MUSICALLESSON) |
| **Project ID** | 1500 |
| **Type** | Passive |
| **Max Level** | 10 |
| **Prerequisites** | None |

| Level | Instrument ATK | Move Speed in Song |
|-------|---------------|-------------------|
| 1 | +3 | +5% of normal |
| 2 | +6 | +10% |
| 3 | +9 | +15% |
| 4 | +12 | +20% |
| 5 | +15 | +25% |
| 6 | +18 | +30% |
| 7 | +21 | +35% |
| 8 | +24 | +40% |
| 9 | +27 | +45% |
| 10 | +30 | +50% |

**Effect on Songs:** Music Lessons level is a variable in every song formula. It adds to FLEE (Whistle), ASPD (Assassin Cross), cast reduction (Bragi), MaxHP (Apple of Idun), and Dissonance damage. See individual song formulas below.

**Implementation:** Passive -- handled by `getPassiveSkillBonuses()`. Adds `instrumentATK` and `musicLessonsLv` to bonus object.

---

### 4.2 Musical Strike / Melody Strike (Offensive)

| Field | Value |
|-------|-------|
| **rA ID** | 316 (BA_MUSICALSTRIKE) |
| **Project ID** | NOT YET DEFINED (missing from ro_skill_data_2nd.js) |
| **Type** | Active, Physical, Single Target |
| **Max Level** | 5 |
| **Range** | 9 cells (450 UE units) |
| **Element** | Weapon/Arrow property |
| **Cast Time** | 1500 ms (pre-renewal) |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Music Lessons Lv 3 |
| **Requires** | Instrument + 1 Arrow |
| **Usable During Performance** | YES |

| Level | ATK% | SP Cost |
|-------|------|---------|
| 1 | 150% | 1 |
| 2 | 175% | 3 |
| 3 | 200% | 5 |
| 4 | 225% | 7 |
| 5 | 250% | 9 |

**Formula:** `(125 + 25 * SkillLv)%` ATK

**SP Cost Formula:** `(SkillLv * 2) - 1`

**Special:** Can be used DURING an active performance without canceling it. This is the Bard's primary offensive skill while singing. Requires instrument weapon and consumes 1 arrow per use.

---

### 4.3 Adaptation to Circumstances / Amp (Utility)

| Field | Value |
|-------|-------|
| **rA ID** | 304 (BD_ADAPTATION) |
| **Project ID** | 1503 |
| **Type** | Active, Self |
| **Max Level** | 1 |
| **SP Cost** | 1 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | None |

**Effect (Pre-Renewal):** Cancels the caster's current active song, dance, or ensemble performance. The same song cannot be immediately re-performed.

**Alternative Cancel Method:** Switching to a dagger weapon instantly cancels any active performance without using this skill or its SP cost. This is the preferred technique in competitive play.

---

### 4.4 Encore (Utility)

| Field | Value |
|-------|-------|
| **rA ID** | 305 (BD_ENCORE) |
| **Project ID** | 1504 |
| **Type** | Active, Self |
| **Max Level** | 1 |
| **SP Cost** | 1 (but the repeated song costs half its normal SP) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 10000 ms |
| **Prerequisites** | Adaptation Lv 1 |
| **Requires** | Instrument (Bard) or Whip (Dancer) |

**Effect:** Re-casts the last performed song/dance at 50% of its original SP cost. If no song was previously performed, Encore fails.

**Implementation:** Server tracks `player.lastPerformanceSkillId` and `player.lastPerformanceLevel`. Encore re-activates that skill with `spCost = Math.ceil(originalSpCost / 2)`.

---

### 4.5 Dissonance / Unchained Serenade (Offensive Performance)

| Field | Value |
|-------|-------|
| **rA ID** | 317 (BA_DISSONANCE) |
| **Project ID** | 1505 |
| **Type** | Active, Performance |
| **Max Level** | 5 |
| **AoE** | 7x7 cells (pre-renewal) |
| **Element** | Neutral (non-elemental magic damage) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Music Lessons Lv 1, Adaptation Lv 1 |
| **Requires** | Instrument equipped |
| **Duration** | 30 seconds |
| **SP Drain** | 1 SP / 3 seconds |

| Level | Damage Per Tick | SP Cost |
|-------|----------------|---------|
| 1 | 40 | 18 |
| 2 | 50 | 21 |
| 3 | 60 | 24 |
| 4 | 70 | 27 |
| 5 | 80 | 30 |

**Damage Formula (Pre-Renewal):** `(30 + 10 * SkillLv)` flat damage per tick, every 3 seconds for 30 seconds (11 ticks total). Music Lessons adds `+MusicLessonsLv * 3` per tick.

**Full Formula:** `DamagePerTick = 30 + 10 * SkillLv + MusicLessonsLv * 3`

**SP Cost Formula:** `15 + SkillLv * 3`

**Mechanics:**
- Non-elemental magic damage -- ignores physical DEF, reduced by MDEF
- Hits all enemies in 7x7 AoE every 3 seconds
- This is a performance skill -- Bard enters performance state
- Required at Lv 3 for all 4 main Bard songs (Whistle, Assassin Cross, Bragi, Apple of Idun)

---

### 4.6 Frost Joker / Unbarring Octave (AoE Status)

| Field | Value |
|-------|-------|
| **rA ID** | 318 (BA_FROSTJOKER) |
| **Project ID** | 1506 |
| **Type** | Active, AoE |
| **Max Level** | 5 |
| **AoE** | Screen-wide (all visible entities) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 4000 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Encore Lv 1 |

| Level | Freeze Chance | SP Cost |
|-------|-------------|---------|
| 1 | 20% | 12 |
| 2 | 25% | 14 |
| 3 | 30% | 16 |
| 4 | 35% | 18 |
| 5 | 40% | 20 |

**Freeze Chance Formula:** `(15 + 5 * SkillLv)%`

**SP Cost Formula:** `10 + 2 * SkillLv`

**Mechanics:**
- Affects **EVERYONE on screen** including party members, the Bard himself, enemies, and other players
- Does NOT affect Boss-type monsters
- Freeze duration: ~5 seconds on players, longer on monsters (standard freeze mechanics apply)
- No weapon restriction (can be used with any weapon)
- Dancer counterpart: Scream/Dazzler (Stun instead of Freeze)
- Can be used during an active performance without canceling it

---

### 4.7 A Whistle / Perfect Tablature (Performance Song)

| Field | Value |
|-------|-------|
| **rA ID** | 319 (BA_WHISTLE) |
| **Project ID** | 1507 |
| **Type** | Active, Performance |
| **Max Level** | 10 |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Dissonance Lv 3 |
| **Requires** | Instrument equipped |
| **Duration (Stay)** | 60 seconds |
| **Effect Linger** | ~20 seconds after leaving AoE |
| **SP Drain** | 1 SP / 5 seconds |

| Level | Base FLEE | Base PD | SP Cost |
|-------|----------|---------|---------|
| 1 | +1 | +1 | 24 |
| 2 | +2 | +1 | 28 |
| 3 | +3 | +2 | 32 |
| 4 | +4 | +2 | 36 |
| 5 | +5 | +3 | 40 |
| 6 | +6 | +3 | 44 |
| 7 | +7 | +4 | 48 |
| 8 | +8 | +4 | 52 |
| 9 | +9 | +5 | 56 |
| 10 | +10 | +5 | 60 |

**Pre-Renewal Formulas (from rAthena xmas_present2008 disassembly -- Issue #1808):**

```
FLEE Bonus = SkillLv + floor(AGI / 10) + floor(MusicLessonsLv * 0.5)
Perfect Dodge Bonus = floor((SkillLv + 1) / 2) + floor(LUK / 30) + floor(MusicLessonsLv * 0.2)
```

**SP Cost Formula:** `20 + SkillLv * 4`

**Example:** Bard with Music Lessons 10, AGI 80, LUK 30, A Whistle Lv 10:
- FLEE = 10 + 8 + 5 = +23
- PD = 5 + 1 + 2 = +8

**Affects:** Caster and party members in AoE only. Does NOT affect enemies.

---

### 4.8 Assassin Cross of Sunset / Impressive Riff (Performance Song)

| Field | Value |
|-------|-------|
| **rA ID** | 320 (BA_ASSASSINCROSS) |
| **Project ID** | 1502 |
| **Type** | Active, Performance |
| **Max Level** | 10 |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Dissonance Lv 3 |
| **Requires** | Instrument equipped |
| **Duration (Stay)** | 120 seconds |
| **Effect Linger** | ~20 seconds after leaving AoE |
| **SP Drain** | 1 SP / 3 seconds |

| Level | Base ASPD Bonus | SP Cost |
|-------|----------------|---------|
| 1 | +11% | 38 |
| 2 | +12% | 41 |
| 3 | +13% | 44 |
| 4 | +14% | 47 |
| 5 | +15% | 50 |
| 6 | +16% | 53 |
| 7 | +17% | 56 |
| 8 | +18% | 59 |
| 9 | +19% | 62 |
| 10 | +20% | 65 |

**Pre-Renewal Formula (from rAthena xmas_present2008 disassembly -- Issue #1808):**

```
ASPD Boost = floor(MusicLessonsLv / 2) + floor(AGI / 20) + (SkillLv + 5)
```

Note: This is NOT a percentage -- it is a flat reduction to the ASPD attack interval delay. However, many pre-renewal servers implement it as `(10 + SkillLv)%` ASPD increase as shown in the RateMyServer table. Both interpretations exist. For our implementation, use the percentage approach: `(10 + SkillLv)%` base, plus stat scaling.

**Full ASPD% Formula:** `(10 + SkillLv + floor(AGI / 20) + floor(MusicLessonsLv / 2))%`

**SP Cost Formula:** `35 + SkillLv * 3`

**Stacking Rules:**
- Stacks with Speed Potions
- Does NOT stack with Adrenaline Rush or Two-Hand Quicken

**Affects:** Caster and party members in AoE only.

---

### 4.9 A Poem of Bragi / Magic Strings (Performance Song)

| Field | Value |
|-------|-------|
| **rA ID** | 321 (BA_POEMBRAGI) |
| **Project ID** | 1501 |
| **Type** | Active, Performance |
| **Max Level** | 10 |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Dissonance Lv 3 |
| **Requires** | Instrument equipped |
| **Duration (Stay)** | 180 seconds |
| **Effect Linger** | ~20 seconds after leaving AoE |
| **SP Drain** | 1 SP / 5 seconds |

| Level | Base VCT Reduction | Base ACD Reduction | SP Cost |
|-------|--------------------|-------------------|---------|
| 1 | -3% | -3% | 40 |
| 2 | -6% | -6% | 45 |
| 3 | -9% | -9% | 50 |
| 4 | -12% | -12% | 55 |
| 5 | -15% | -15% | 60 |
| 6 | -18% | -18% | 65 |
| 7 | -21% | -21% | 70 |
| 8 | -24% | -24% | 75 |
| 9 | -27% | -27% | 80 |
| 10 | -30% | -50% (capped) | 85 |

**Pre-Renewal Formulas (from rAthena xmas_present2008 disassembly -- Issue #1808):**

```
VCT Reduction = (SkillLv * 3 + floor(DEX / 10) + MusicLessonsLv) %
ACD Reduction = (SkillLv < 10 ? SkillLv * 3 : 50) + floor(INT / 5) + MusicLessonsLv * 2) %
```

**SP Cost Formula:** `35 + SkillLv * 5`

**Example:** Bard with Music Lessons 10, DEX 99, INT 50, Bragi Lv 10:
- VCT Reduction = (30 + 9 + 10) = 49%
- ACD Reduction = (50 + 10 + 20) = 80% (effectively eliminates most after-cast delays)

**Suffragium interaction:** Suffragium (Priest skill) does NOT stack with Bragi's VCT reduction. They are independent multiplicative effects: `Final VCT = Base * (1 - DEX/150) * (1 - Bragi%) * (1 - Suffragium%)`.

**Affects:** Caster and party members in AoE only.

**This is arguably the most powerful support skill in the game.** A well-built Bard with high DEX/INT can reduce a Wizard's cast time and after-cast delay by 50-80%, dramatically increasing DPS.

---

### 4.10 Apple of Idun / Song of Lutie (Performance Song)

| Field | Value |
|-------|-------|
| **rA ID** | 322 (BA_APPLEIDUN) |
| **Project ID** | 1508 |
| **Type** | Active, Performance |
| **Max Level** | 10 |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Dissonance Lv 3 |
| **Requires** | Instrument equipped |
| **Duration (Stay)** | 180 seconds |
| **Effect Linger** | ~20 seconds after leaving AoE |
| **SP Drain** | 1 SP / 6 seconds |

| Level | Base MaxHP% | Base Heal/Tick | SP Cost |
|-------|-----------|---------------|---------|
| 1 | +7% | 35 HP | 40 |
| 2 | +9% | 40 HP | 45 |
| 3 | +11% | 45 HP | 50 |
| 4 | +13% | 50 HP | 55 |
| 5 | +15% | 55 HP | 60 |
| 6 | +17% | 60 HP | 65 |
| 7 | +19% | 65 HP | 70 |
| 8 | +21% | 70 HP | 75 |
| 9 | +23% | 75 HP | 80 |
| 10 | +25% | 80 HP | 85 |

**Pre-Renewal Formulas (from rAthena xmas_present2008 disassembly -- Issue #1808):**

```
MaxHP Boost = (5 + SkillLv * 2 + floor(VIT / 10) + floor(MusicLessonsLv / 2)) %
HP Recovery Per Tick = (30 + SkillLv * 5) + floor(VIT / 2) + MusicLessonsLv * 5
```

**SP Cost Formula:** `35 + SkillLv * 5`

**Healing Frequency:** Every 6 seconds (31 ticks over 180 seconds)

**Example:** Bard with Music Lessons 10, VIT 50, Apple of Idun Lv 10:
- MaxHP Boost = (5 + 20 + 5 + 5) = 35%
- HP Per Tick = (80) + 25 + 50 = 155 HP every 6 seconds

**Affects:** Caster and party members in AoE only.

---

### 4.11 Pang Voice (Quest Skill)

| Field | Value |
|-------|-------|
| **rA ID** | 1010 (BA_PANGVOICE) |
| **Project ID** | 1509 |
| **Type** | Active, Single Target |
| **Max Level** | 1 |
| **Range** | 9 cells (450 UE units) |
| **SP Cost** | 20 |
| **Cast Time** | 800 ms (0.8s variable) |
| **After-Cast Delay** | 2000 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Quest (Bard Platinum Skill Quest, Job Lv 40+) |

**Effect:** Inflicts Confusion (Chaos) status on a single target.
- Does NOT work on Boss monsters
- Duration: ~15-17 seconds
- Success rate affected by target's INT resistance
- No weapon restriction

---

### 4.12 Ensemble Skills (Bard + Dancer)

All ensemble skills require both a Bard and Dancer in the same party, standing adjacent. The effective skill level = `floor((BardLv + DancerLv) / 2)`.

#### Lullaby (BD_LULLABY, rA 306, Project ID 1530)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **AoE** | 9x9 cells |
| **SP Cost** | 20 |
| **Duration** | 60 seconds |
| **SP Drain** | 1 SP / 4 seconds |
| **Bard Prereq** | A Whistle Lv 10 |
| **Dancer Prereq** | Humming Lv 10 |

**Effect:** Chance to inflict Sleep on all enemies in AoE every 6 seconds (11 ticks). Does not affect Boss monsters. Affected by target's INT resistance.

#### Mr. Kim A Rich Man / Mental Sensing (BD_RICHMANKIM, rA 307, Project ID 1531)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **AoE** | 9x9 cells |
| **SP Cost** | 20 (pre-renewal, flat) |
| **Duration** | 60 seconds |
| **SP Drain** | 1 SP / 3 seconds |
| **Prereq** | Invulnerable Siegfried Lv 3 |

| Level | EXP Bonus |
|-------|-----------|
| 1 | +136% |
| 2 | +147% |
| 3 | +158% |
| 4 | +169% |
| 5 | +180% |

**Note:** Pre-renewal EXP values are much higher than renewal (20-60%). These are total EXP multipliers (136% means 1.36x EXP).

#### Eternal Chaos / Down Tempo (BD_ETERNALCHAOS, rA 308, Project ID 1532)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **AoE** | 9x9 cells |
| **SP Cost** | 30 |
| **Duration** | 60 seconds |
| **Prereq** | Loki's Veil Lv 1 |

**Effect:** Reduces DEF of all enemies in AoE to 0. Cannot be dispelled. Extremely powerful in PvP/WoE.

#### A Drum on the Battlefield / Battle Theme (BD_DRUMBATTLEFIELD, rA 309, Project ID 1533)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **AoE** | 9x9 cells |
| **SP Cost** | 38-50 |
| **Duration** | 60 seconds |
| **Bard Prereq** | Apple of Idun Lv 10 |
| **Dancer Prereq** | Service for You Lv 10 |

| Level | ATK Bonus | DEF Bonus | SP Cost |
|-------|-----------|-----------|---------|
| 1 | +50 | +4 | 38 |
| 2 | +75 | +6 | 41 |
| 3 | +100 | +8 | 44 |
| 4 | +125 | +10 | 47 |
| 5 | +150 | +12 | 50 |

**Formula:** ATK = `25 + 25 * SkillLv`, DEF = `2 + 2 * SkillLv`. Cannot be dispelled.

#### The Ring of Nibelungen / Harmonic Lick (BD_RINGNIBELUNGEN, rA 310, Project ID 1534)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **AoE** | 9x9 cells |
| **SP Cost** | 38-50 |
| **Duration** | 60 seconds |
| **Prereq** | Drum on Battlefield Lv 3 |

| Level | Lv4 Weapon ATK Bonus | SP Cost |
|-------|---------------------|---------|
| 1 | +75 | 38 |
| 2 | +100 | 41 |
| 3 | +125 | 44 |
| 4 | +150 | 47 |
| 5 | +175 | 50 |

**Formula:** ATK bonus = `50 + 25 * SkillLv`. Only applies to characters wielding Level 4 weapons. DEF-ignoring flat ATK bonus. Cannot be dispelled.

#### Loki's Veil / Classical Pluck (BD_ROKISWEIL, rA 311, Project ID 1535)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **AoE** | 9x9 cells |
| **SP Cost** | 15 |
| **Duration** | 60 seconds |
| **SP Drain** | 1 SP / 4 seconds |
| **Bard Prereq** | Assassin Cross Lv 10 |
| **Dancer Prereq** | Please Don't Forget Me Lv 10 |

**Effect:** Prevents ALL skill usage for everything within AoE, including the performers' party members. Cannot be dispelled. Extremely powerful in WoE/PvP for zone denial.

#### Into the Abyss / Power Cord (BD_INTOABYSS, rA 312, Project ID 1536)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **AoE** | 9x9 cells (pre-renewal) |
| **SP Cost** | 10 |
| **Duration** | 60 seconds |
| **SP Drain** | 1 SP / 5 seconds |
| **Prereq** | Lullaby Lv 1 |

**Effect:** Party members in AoE can cast spells without gemstone requirements and lay traps without item cost. Cannot be dispelled.

#### Invulnerable Siegfried / Acoustic Rhythm (BD_SIEGFRIED, rA 313, Project ID 1537)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **AoE** | 9x9 cells |
| **SP Cost** | 20 (pre-renewal, flat) |
| **Duration** | 60 seconds |
| **SP Drain** | 1 SP / 3 seconds |
| **Bard Prereq** | Poem of Bragi Lv 10 |
| **Dancer Prereq** | Fortune's Kiss Lv 10 |

| Level | Elemental Resist | Status Resist |
|-------|-----------------|---------------|
| 1 | +60% | +10% |
| 2 | +65% | +20% |
| 3 | +70% | +30% |
| 4 | +75% | +40% |
| 5 | +80% | +50% |

**Formula:** Element Resist = `55 + 5 * SkillLv`, Status Resist = `SkillLv * 10`. Cannot be dispelled.

---

## 5. Skill Tree and Prerequisites

### Bard Skill Tree Layout

```
Row 0: [Music Lessons]   [Adaptation]   [Encore]
Row 1: [A Whistle]       [Bragi]        [Assassin Cross]
Row 2: [Dissonance]      [Apple of Idun] [Frost Joker]
Row 3: [Pang Voice*]     [Musical Strike]
Row 4: [Lullaby**]       [Mr.Kim**]     [Eternal Chaos**] [Siegfried**]
Row 5: [Drum**]          [Nibelungen**] [Loki's Veil**]   [Into Abyss**]
```

`*` = Quest skill, `**` = Ensemble skill

### Prerequisite Chain (Bard Side)

```
Music Lessons 1 -> Dissonance 1
                -> Adaptation 1 -> Encore 1 -> Frost Joker

Music Lessons 3 -> Musical Strike
Music Lessons 3 -> (via Dissonance 3):
    Dissonance 3 -> A Whistle
                 -> Assassin Cross of Sunset
                 -> A Poem of Bragi
                 -> Apple of Idun

A Whistle 10 -> Lullaby (ensemble)
Assassin Cross 10 -> Loki's Veil (ensemble)
Poem of Bragi 10 -> Invulnerable Siegfried (ensemble)
Apple of Idun 10 -> A Drum on the Battlefield (ensemble)
Lullaby 1 -> Into the Abyss (ensemble)
Drum on Battlefield 3 -> Ring of Nibelungen (ensemble)
Invulnerable Siegfried 3 -> Mr. Kim A Rich Man (ensemble)
Loki's Veil 1 -> Eternal Chaos (ensemble)
```

### Inherited Archer Skills

Bards inherit ALL Archer skills: Owl's Eye, Vulture's Eye, Improve Concentration, Double Strafe, Arrow Shower, Arrow Crafting (quest), Arrow Repel (quest).

---

## 6. Existing Code Audit

### Current State in `ro_skill_data_2nd.js`

Bard skills ARE defined (IDs 1500-1537). Here is a complete audit of each skill definition against canonical data:

#### 1500: Music Lessons -- CORRECT (minor)
- ATK bonus: CORRECT (+3/level)
- effectValue formula: CORRECT (`(i+1)*3`)
- Missing: No `instrumentATK` semantic -- just uses generic effectValue. Acceptable for passive.

#### 1501: A Poem of Bragi -- ISSUES FOUND
- SP Cost: `20+i*5` = 20,25,30...65. **WRONG.** Canonical: `35+i*5` = 40,45,50...85
- effectValue: `3+i*3` = 3,6,9...27,30. Unclear what this represents. Base VCT reduction should be `3*SkillLv` = 3,6,9...30. The `3+i*3` pattern gives 3,6,9...30 which matches. OK.
- Prerequisites: `Music Lessons Lv 3`. **WRONG.** Canonical: Dissonance Lv 3
- `isPerformance: true`: Good, this flag exists

#### 1502: Assassin Cross of Sunset -- ISSUES FOUND
- SP Cost: `38+i*2` = 38,40,42...56. **WRONG.** Canonical: `35+3*SkillLv` = 38,41,44...65
- effectValue: `i+1` = 1,2,3...10. Unclear semantics -- should represent ASPD boost. Base formula is `(10+SkillLv)%` so effectValue should encode the full boost percentage.
- Prerequisites: `Music Lessons Lv 3`. **WRONG.** Canonical: Dissonance Lv 3

#### 1503: Adaptation -- CORRECT
- SP Cost: 1. CORRECT.
- Prerequisites: None. CORRECT.

#### 1504: Encore -- PARTIALLY CORRECT
- SP Cost: 1. CORRECT (the 1 SP is Encore's own cost; the repeated song costs half).
- Prerequisites: Adaptation Lv 1. CORRECT.
- Missing: cooldown should be 10000 ms (10 seconds)

#### 1505: Dissonance -- ISSUES FOUND
- SP Cost: `18+i*3` = 18,21,24,27,30. CORRECT.
- effectValue: `30+i*10` = 30,40,50,60,70. **WRONG.** Canonical damage is `30+10*SkillLv` = 40,50,60,70,80 per tick. The 0-indexed formula should be `40+(i)*10`.
- Prerequisites: `Music Lessons Lv 1`. **INCOMPLETE.** Should also require `Adaptation Lv 1`.
- `isPerformance: true`: Good.

#### 1506: Frost Joker -- ISSUES FOUND
- SP Cost: `12+i*2` = 12,14,16,18,20. CORRECT.
- effectValue: `10+i*5` = 10,15,20,25,30. **WRONG.** Canonical freeze chance is `(15+5*SkillLv)%` = 20,25,30,35,40. Formula should be `15+(i+1)*5` = 20,25,30,35,40.
- afterCastDelay: missing. **WRONG.** Should be 4000 ms.
- Prerequisites: Encore Lv 1. CORRECT.

#### 1507: A Whistle -- ISSUES FOUND
- SP Cost: `24+i*4` = 24,28,32...60. CORRECT (matches `20+4*SkillLv`).
- effectValue: `3+i*2` = 3,5,7...21. **WRONG/UNCLEAR.** Base FLEE is `SkillLv` = 1-10. The effectValue doesn't match any canonical formula.
- Prerequisites: `Music Lessons Lv 3`. **WRONG.** Canonical: Dissonance Lv 3.

#### 1508: Apple of Idun -- ISSUES FOUND
- SP Cost: `40+i*5` = 40,45,50...85. CORRECT.
- effectValue: `3+i*2` = 3,5,7...21. **WRONG/UNCLEAR.** Base MaxHP% is `(5+2*SkillLv)%` = 7,9,11...25. The formula should encode the MaxHP percentage.
- Prerequisites: `Music Lessons Lv 10`. **WRONG.** Canonical: Dissonance Lv 3. Music Lessons 10 is not a prerequisite for Apple of Idun.

#### 1509: Pang Voice -- PARTIALLY CORRECT
- SP Cost: 20. CORRECT.
- Range: 450. CORRECT (9 cells).
- Missing: castTime should be 800 ms. afterCastDelay should be 2000 ms.
- Cooldown: 3000 ms. **WRONG.** Should be 0 (the 3s figure may be confused with afterCastDelay).

#### Musical Strike -- MISSING
- Not defined in `ro_skill_data_2nd.js` at all. Needs to be added.

#### 1510: Unbarring Octave -- WRONG SKILL
- This appears to be a conflation. "Unbarring Octave" is the iRO name for Frost Joker (rA 318). It should not be a separate skill. The current data has it as a separate performance skill preventing skill use in AoE -- this is actually Loki's Veil behavior. **Remove or repurpose.**

#### 1530-1537: Ensemble Skills -- MULTIPLE ISSUES
- All ensembles are defined as `maxLevel: 5`. **SOME WRONG.** Lullaby is maxLevel 1, Eternal Chaos is maxLevel 1, Loki's Veil is maxLevel 1, Into the Abyss is maxLevel 1. Only Mr. Kim (5), Drum (5), Nibelungen (5), and Siegfried (5) have maxLevel 5.
- SP costs vary widely from canonical values.
- Missing `isEnsemble` checks and partner proximity requirements.
- Prerequisites are all `Music Lessons Lv X` instead of the correct ensemble prerequisites (e.g., Lullaby requires A Whistle Lv 10, not Music Lessons 10).

### Summary of Audit Findings

| Category | Count |
|----------|-------|
| Skills correctly defined | 2 (Music Lessons, Adaptation) |
| Skills with SP cost errors | 2 (Bragi, Assassin Cross) |
| Skills with wrong prerequisites | 5 (Bragi, Assassin Cross, Whistle, Apple of Idun, Dissonance) |
| Skills with wrong effectValue | 4 (Frost Joker, Whistle, Apple of Idun, Dissonance) |
| Skills with missing timing values | 3 (Encore missing cooldown, Frost Joker missing ACD, Pang Voice missing cast/ACD) |
| Skills with wrong maxLevel | 4 (ensemble skills) |
| Missing skills | 1 (Musical Strike) |
| Duplicate/wrong skills | 1 (Unbarring Octave -- should not exist as separate from Frost Joker) |

---

## 7. New Systems Required

### 7.1 Performance System (Priority: HIGH)

**Scope:** Server-side state machine for active song performance.

**Required components:**
1. `player.performanceState` object (see Section 2)
2. Performance activation handler in `skill:use` (start song, enter performance state, spawn ground AoE)
3. Performance tick loop (every 1 second: drain SP, check AoE occupants, apply/refresh buffs)
4. Performance cancellation logic (Adaptation, weapon swap, SP exhaustion, CC effects)
5. Movement speed modifier during performance (based on Music Lessons level)
6. Song buff application system (apply song-specific buff to party members in AoE)
7. Song effect linger (20-second buff that refreshes while in AoE)
8. Song stacking rules (only 1 active song per Bard, no same-song double-stack from 2 Bards)

**New Socket.io events:**
- `performance:start` -- broadcast to zone (caster enters performance state, AoE visible)
- `performance:end` -- broadcast to zone (AoE removed)
- `performance:buff_applied` -- to individual players entering AoE
- `performance:buff_removed` -- to individual players leaving AoE (after linger expires)

**New buff types needed:**
- `song_whistle` -- FLEE + Perfect Dodge bonus
- `song_assassin_cross` -- ASPD bonus
- `song_bragi` -- VCT reduction + ACD reduction
- `song_apple_of_idun` -- MaxHP% + HP regen tick
- `song_dissonance` -- Damage tick (applied to enemies, not a buff per se)

### 7.2 Ensemble System (Priority: LOW -- Deferred)

**Depends on:** Party system, Performance system

**Required components:**
1. Party proximity check (Bard+Dancer within 1-2 cells)
2. Synchronized dual-performer state
3. Averaged skill level calculation
4. 9x9 AoE centered between both performers
5. Dual SP drain
6. Dual cancellation logic

**Recommendation:** Defer until party system is fully implemented. Ensemble skills can remain defined but unactivatable.

### 7.3 Instrument Weapon Type (Priority: HIGH)

The `weaponType` field needs to support `'instrument'` as a valid weapon type. This affects:
- Weapon mastery checks (Music Lessons ATK bonus only with instruments)
- Performance skill requirements (songs require instrument equipped)
- Musical Strike weapon check
- ASPD calculations (instruments need base delay values)

**Required changes:**
- Add `'instrument'` to valid weapon types in item data
- Add `'instrument'` to `ASPD_BASE_DELAYS` table (Bard-specific entry)
- Music Lessons passive: check `weaponType === 'instrument'` before applying ATK bonus

### 7.4 Performance Movement Speed Modifier (Priority: MEDIUM)

When a Bard is in performance state, their movement speed must be reduced:
- Without Music Lessons: cannot move at all
- With Music Lessons N: `moveSpeed = baseSpeed * (N * 0.05)`

This requires the position update handler to either:
- Block position updates entirely if performanceState.active and musicLessonsLv === 0
- Apply a speed multiplier to validate position deltas if musicLessonsLv > 0

### 7.5 Song AoE Tick System (Priority: HIGH)

Songs create persistent ground-based AoE effects that tick periodically:
- Buff songs (Whistle, Assassin Cross, Bragi, Apple of Idun): Check every 1 second for party members in AoE, apply/refresh their song buff
- Damage songs (Dissonance): Deal damage every 3 seconds to enemies in AoE
- Apple of Idun: Heal tick every 6 seconds in addition to MaxHP buff

This is similar to the existing Fire Wall ground effect system but with different behavior (buffs instead of damage).

---

## 8. Implementation Priority

### Phase 1: Foundation (Required for any Bard gameplay)

1. **Add `instrument` weapon type** to weapon type system
2. **Implement Music Lessons passive** in `getPassiveSkillBonuses()`
3. **Add Musical Strike** skill definition and handler (physical single-target, 1500ms cast time)
4. **Fix existing skill definitions** (SP costs, prerequisites, effectValues, timing values)
5. **Implement Frost Joker** handler (screen-wide freeze chance, 4s ACD)
6. **Implement Pang Voice** handler (single target Confusion, 800ms cast, 2s ACD)

### Phase 2: Performance System Core

7. **Implement performance state machine** (start/cancel/tick)
8. **Implement Dissonance** as first performance skill (damage ticks)
9. **Implement A Whistle** performance (FLEE/PD buff)
10. **Implement Apple of Idun** performance (MaxHP/heal tick)
11. **Implement Assassin Cross of Sunset** performance (ASPD buff)
12. **Implement A Poem of Bragi** performance (VCT/ACD reduction)
13. **Implement Adaptation** handler (cancel performance)
14. **Implement Encore** handler (replay last song at half SP)

### Phase 3: Polish and Integration

15. **Performance movement speed restriction**
16. **Song buff linger system** (20s after leaving AoE)
17. **Song stacking rules** (1 song per Bard, no double-stack)
18. **Client-side VFX** for active performances (ground effect visualization)
19. **Client-side UI** for performance state (show active song, SP drain)

### Phase 4: Ensemble System (Deferred)

20. **Party proximity detection**
21. **Ensemble activation/cancellation**
22. **All 8 ensemble skill handlers**

---

## 9. Integration Points

### With Existing Buff System (`applyBuff`, `getBuffStatModifiers`)

Song buffs integrate with the existing buff system. Each song creates a buff on affected party members:

| Song | Buff Name | Buff Fields |
|------|-----------|-------------|
| A Whistle | `song_whistle` | `fleeBonus`, `perfectDodgeBonus` |
| Assassin Cross | `song_assassin_cross` | `aspdBonus` (percentage) |
| Poem of Bragi | `song_bragi` | `castReduction` (%), `acdReduction` (%) |
| Apple of Idun | `song_apple_of_idun` | `maxHpPercent`, `hpRegenPerTick` |
| Dissonance | (no buff -- direct damage) | N/A |

The `getBuffStatModifiers()` function needs to read these song buff fields and include them in the stat modifiers that affect combat calculations.

### With Cast Time System

Poem of Bragi's VCT reduction must integrate with `calculateActualCastTime()`:

```js
function calculateActualCastTime(baseCastTime, casterDex, bragiReduction) {
    if (baseCastTime <= 0) return 0;
    let time = baseCastTime * Math.max(0, 1 - casterDex / 150);
    if (bragiReduction > 0) {
        time = time * Math.max(0, 1 - bragiReduction / 100);
    }
    return Math.max(0, Math.floor(time));
}
```

### With ASPD System

Assassin Cross of Sunset's ASPD boost must integrate with `calculateASPD()`. The boost is a percentage reduction to attack delay:

```js
// In ASPD calculation, after base ASPD is computed:
if (songAspdBoost > 0) {
    aspd = aspd * (1 + songAspdBoost / 100);
    aspd = Math.min(aspd, 190);  // Pre-renewal cap
}
```

### With After-Cast Delay System

Poem of Bragi's ACD reduction must integrate with `applySkillDelays()`:

```js
function applySkillDelays(characterId, player, skillId, levelData, socket) {
    let acdMs = levelData.afterCastDelay || 0;

    // Apply Bragi reduction
    const bragiMod = getBuffStatModifiers(player);
    if (bragiMod.acdReduction > 0) {
        acdMs = Math.floor(acdMs * (1 - bragiMod.acdReduction / 100));
    }

    // ... rest of function
}
```

### With Weight System

Standard weight threshold checks apply to all Bard skills:
- >= 90% weight: cannot use skills (including starting songs)
- >= 50% weight: no SP regen (affects sustaining songs)

### With Status Effect System

- Frozen/Stoned/Stunned/Silenced: Cannot start or maintain a performance. If a Bard is CC'd during a performance, the performance auto-cancels.
- Frost Joker applies the standard Freeze status effect.
- Pang Voice applies the standard Confusion status effect.

---

## 10. Data Tables and Constants

### Bard-Specific Constants

```js
// Performance system constants
const PERFORMANCE_CONSTANTS = {
    SOLO_AOE_CELLS: 7,                    // 7x7 cells for solo songs
    SOLO_AOE_RADIUS: 175,                 // 3.5 cells * 50 UE units
    ENSEMBLE_AOE_CELLS: 9,               // 9x9 cells for ensembles
    ENSEMBLE_AOE_RADIUS: 225,            // 4.5 cells * 50 UE units
    EFFECT_LINGER_DURATION: 20000,        // 20 second buff linger after leaving AoE
    PERFORMANCE_CHECK_INTERVAL: 1000,     // Check AoE occupants every 1 second
    ENSEMBLE_MAX_DISTANCE: 100,           // Max 2 cells (100 UE units) between Bard and Dancer
};

// SP drain intervals per song (milliseconds between 1 SP drain)
const SONG_SP_DRAIN = {
    'a_whistle': 5000,
    'assassin_cross_of_sunset': 3000,
    'poem_of_bragi': 5000,
    'apple_of_idun': 6000,
    'dissonance': 3000,
    // Ensemble SP drain intervals
    'lullaby': 4000,
    'mr_kim_a_rich_man': 3000,
    'eternal_chaos': 3000,
    'drum_on_battlefield': 3000,
    'ring_of_nibelungen': 3000,
    'lokis_veil': 4000,
    'into_the_abyss': 5000,
    'invulnerable_siegfried': 3000,
};
```

### ASPD Base Delays for Instrument

Add to `ASPD_BASE_DELAYS` in `ro_exp_tables.js`:

```js
// Bard ASPD base delays with instruments
// Format: { weaponType: delay }
// Bard's instrument delay is similar to bow but slightly slower
'bard': {
    'unarmed': 650,
    'dagger': 600,
    'instrument': 700,  // Primary weapon
    'bow': 650,          // Inherited from Archer
},
```

### Corrected Skill Definitions

The following corrections should be applied to `ro_skill_data_2nd.js`:

```js
// Musical Strike -- NEW (add between 1509 and 1510)
{ id: 1511, name: 'musical_strike', displayName: 'Musical Strike', classId: 'bard',
  maxLevel: 5, type: 'active', targetType: 'single', element: 'neutral',
  range: 450, description: 'Ranged attack with instrument. Usable during performance.',
  icon: 'musical_strike', treeRow: 3, treeCol: 1,
  prerequisites: [{ skillId: 1500, level: 3 }],
  requiresWeapon: 'instrument',
  usableDuringPerformance: true,
  levels: genLevels(5, i => ({
      level: i+1, spCost: i*2+1, castTime: 1500,
      afterCastDelay: 0, cooldown: 0,
      effectValue: 150+i*25, duration: 0
  }))
},

// Fix Poem of Bragi SP costs and prerequisites
// spCost: 40,45,50,55,60,65,70,75,80,85  (was 20,25,30...)
// prerequisites: Dissonance Lv 3 (was Music Lessons Lv 3)

// Fix Assassin Cross of Sunset SP costs and prerequisites
// spCost: 38,41,44,47,50,53,56,59,62,65  (was 38,40,42...)
// prerequisites: Dissonance Lv 3 (was Music Lessons Lv 3)

// Fix A Whistle prerequisites
// prerequisites: Dissonance Lv 3 (was Music Lessons Lv 3)

// Fix Apple of Idun prerequisites
// prerequisites: Dissonance Lv 3 (was Music Lessons Lv 10)

// Fix Dissonance prerequisites
// Add: Adaptation Lv 1 (currently only requires Music Lessons Lv 1)

// Fix Frost Joker effectValue and add afterCastDelay
// effectValue: 20,25,30,35,40 (was 10,15,20,25,30)
// afterCastDelay: 4000 (was missing)

// Fix Pang Voice timing
// castTime: 800 (was 0)
// afterCastDelay: 2000 (was missing)
// cooldown: 0 (was 3000)

// Fix ensemble maxLevels
// Lullaby: maxLevel 1 (was 5)
// Eternal Chaos: maxLevel 1 (was 5)
// Loki's Veil: maxLevel 1 (was 5)
// Into the Abyss: maxLevel 1 (was 5)

// Remove Unbarring Octave (1510) -- duplicate of Frost Joker
```

### Song Effect Calculation Functions

```js
// Pre-renewal song effect formulas (from rAthena xmas_present2008 disassembly)

function calculateWhistleEffect(skillLv, casterAGI, casterLUK, musicLessonsLv) {
    return {
        flee: skillLv + Math.floor(casterAGI / 10) + Math.floor(musicLessonsLv * 0.5),
        perfectDodge: Math.floor((skillLv + 1) / 2) + Math.floor(casterLUK / 30) + Math.floor(musicLessonsLv * 0.2),
    };
}

function calculateAssassinCrossEffect(skillLv, casterAGI, musicLessonsLv) {
    return {
        aspdBoost: Math.floor(musicLessonsLv / 2) + Math.floor(casterAGI / 20) + (skillLv + 5),
    };
}

function calculateBragiEffect(skillLv, casterDEX, casterINT, musicLessonsLv) {
    return {
        vctReduction: skillLv * 3 + Math.floor(casterDEX / 10) + musicLessonsLv,
        acdReduction: (skillLv < 10 ? skillLv * 3 : 50) + Math.floor(casterINT / 5) + musicLessonsLv * 2,
    };
}

function calculateAppleOfIdunEffect(skillLv, casterVIT, musicLessonsLv) {
    return {
        maxHpPercent: 5 + skillLv * 2 + Math.floor(casterVIT / 10) + Math.floor(musicLessonsLv / 2),
        hpPerTick: (30 + skillLv * 5) + Math.floor(casterVIT / 2) + musicLessonsLv * 5,
    };
}

function calculateDissonanceDamage(skillLv, musicLessonsLv) {
    return 30 + 10 * skillLv + musicLessonsLv * 3;
}
```

---

## Sources

- [Bard - iRO Wiki Classic](https://irowiki.org/classic/Bard)
- [Bard - iRO Wiki](https://irowiki.org/wiki/Bard)
- [Bard Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=19)
- [Music Lessons - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_MUSICALLESSON)
- [Dissonance - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_DISSONANCE)
- [A Whistle - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_WHISTLE)
- [Assassin Cross of Sunset - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_ASSASSINCROSS)
- [Poem of Bragi - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_POEMBRAGI)
- [Apple of Idun - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_APPLEIDUN)
- [Musical Strike - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_MUSICALSTRIKE)
- [Frost Joker - rAthena Pre-Re DB](https://pre.pservero.com/skill/BA_FROSTJOKER)
- [Music Lessons - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=315)
- [Musical Strike - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=316)
- [Dissonance - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=317)
- [Frost Joker - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=318)
- [A Whistle - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=319)
- [Assassin Cross of Sunset - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=320)
- [Poem of Bragi - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=321)
- [Apple of Idun - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=322)
- [Lullaby - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=306)
- [Mr. Kim A Rich Man - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=307)
- [Eternal Chaos - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=308)
- [A Drum on the Battlefield - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=309)
- [Ring of Nibelungen - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=310)
- [Loki's Veil - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=311)
- [Into the Abyss - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=312)
- [Invulnerable Siegfried - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=313)
- [Pang Voice - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=1010)
- [Song stat effects issue (pre-renewal formulas) - rAthena GitHub #1808](https://github.com/rathena/rathena/issues/1808)
- [rAthena pre-re skill_tree.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_tree.yml)
- [rAthena pre-re skill_db.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [Music Lessons - iRO Wiki](https://irowiki.org/wiki/Music_Lessons)
- [Bard/Dancer Guide - GameFAQs (aldoteng)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/35759)
- [Bard Guide - GameFAQs (Jath)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/30940)
- [Bard/Dancer Guide - myrolife.blogspot](http://myrolife.blogspot.com/2016/05/barddancerclowngypsy-guide.html)
- [Poem of Bragi formula - rAthena GitHub #3169](https://github.com/rathena/rathena/issues/3169)
- [Assassin Cross base values - rAthena GitHub #7726](https://github.com/rathena/rathena/issues/7726)
