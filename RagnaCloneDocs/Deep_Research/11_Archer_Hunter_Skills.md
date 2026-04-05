# Archer -> Hunter Skills -- Deep Research

> **Sources:** iRO Wiki Classic, iRO Wiki, RateMyServer, rAthena pre-renewal source (`battle.cpp`, `skill.cpp`, `status.cpp`, `db/pre-re/skill_db.yml`, `db/pre-re/status.yml`), GameFAQs Trap Hunter Guide, RagnaPlace, Project Alfheim Wiki, Ragnarok Fandom Wiki, Origins RO Wiki
> **Scope:** All pre-renewal (classic) mechanics. NO renewal data unless explicitly noted.
> **Project Skill IDs:** Archer 300-306, Hunter 900-917

---

## Table of Contents

1. [Archer Skills (IDs 300-306)](#1-archer-skills-ids-300-306)
2. [Hunter Skills (IDs 900-917)](#2-hunter-skills-ids-900-917)
3. [Arrow System](#3-arrow-system)
4. [Falcon System](#4-falcon-system)
5. [Trap System](#5-trap-system)
6. [Ankle Snare Special Mechanics](#6-ankle-snare-special-mechanics)
7. [Arrow Crafting](#7-arrow-crafting)
8. [Skill Trees and Prerequisites](#8-skill-trees-and-prerequisites)
9. [Implementation Checklist](#9-implementation-checklist)
10. [Gap Analysis](#10-gap-analysis)

---

## 1. Archer Skills (IDs 300-306)

### 1.1 Owl's Eye (ID 300, rA: AC_OWL)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All |
| Max Level | 10 | All |
| Prerequisites | None | All |
| Effect | +1 DEX per level | iRO Wiki Classic, rAthena |

**Formula:** `bonusDEX = SkillLv` (flat +1 to +10)

No weapon restriction. Always active. The DEX bonus is treated as a passive stat bonus (same category as job bonuses) for purposes of Improve Concentration's percentage calculation.

---

### 1.2 Vulture's Eye (ID 301, rA: AC_VULTURE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All |
| Max Level | 10 | All |
| Prerequisites | Owl's Eye Lv 3 | All |
| Range Bonus | +1 cell per level (bow only) | iRO Wiki Classic |
| HIT Bonus | +1 flat HIT per level (bow only) | iRO Wiki Classic, rAthena |
| Weapon restriction | Bow class weapons only | All |

**Formulas:**
```
bonusRange = SkillLv (cells), bow only
bonusHIT = SkillLv (flat), bow only
```

The HIT bonus does not appear in the Status Window according to iRO Wiki. The range bonus extends both auto-attack range and skill range for bow skills (Double Strafe, Arrow Shower, Arrow Repel, Blitz Beat). Rogue class also has access to this skill.

---

### 1.3 Improve Concentration (ID 302, rA: AC_CONCENTRATION)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self Buff | All |
| Max Level | 10 | All |
| Prerequisites | Vulture's Eye Lv 1 | All |
| Cast Time | 0 (instant) | rAthena pre-re |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |

**Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| AGI/DEX +% | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Duration (s) | 60 | 80 | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |
| SP Cost | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 65 | 70 |

**Formulas:**
```
BonusPercent = (2 + SkillLv)%
Duration = (40 + 20 * SkillLv) seconds
SP Cost = (20 + 5 * SkillLv)
```

**CRITICAL -- Stat Source Filtering:**
The percentage applies ONLY to specific stat sources (confirmed by iRO Wiki Classic):
- Base AGI/DEX (player-allocated stat points)
- Job bonus AGI/DEX
- Equipment base AGI/DEX (e.g., Tights)
- Owl's Eye passive DEX bonus

The percentage does NOT apply to:
- Card bonuses (e.g., Zerom Card DEX)
- Other buff bonuses (Blessing, Increase AGI, etc.)
- Enchant bonuses
- Item refine bonuses
- Item set bonuses

**iRO Wiki Classic quote:** "will not be affected by DEX received from cards and buffs (bonus DEX received from Job bonuses, armor and Owl's Eye are factored in, though)"

**Reveal Hidden (one-time on cast):**
- Detects hidden/cloaked characters within a 3x3 area (3-cell radius) around caster
- One-time detection on activation, NOT persistent
- Removes Hiding, Cloaking, and Chase Walk from detected targets

---

### 1.4 Double Strafe (ID 303, rA: AC_DOUBLE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged | All |
| Max Level | 10 | All |
| Prerequisites | None (Archer) / Vulture's Eye Lv 10 (Rogue) | rAthena |
| SP Cost | 12 (all levels) | All |
| Cast Time | 0 (instant) | rAthena pre-re |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Range | 9 cells base + Vulture's Eye bonus | rAthena pre-re |
| Element | Weapon/Arrow element | rAthena |
| Weapon Required | Bow | All |
| Arrow Consumed | 1 per cast | All |
| Target | Single enemy | All |

**Damage Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Per-hit ATK% | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 |
| Total ATK% | 200 | 220 | 240 | 260 | 280 | 300 | 320 | 340 | 360 | 380 |

**Formulas:**
```
Per-hit = (90 + 10 * SkillLv)%
Total = 2 * Per-hit
```

**CRITICAL -- Hit Structure (Bundled Damage):**
iRO Wiki explicitly states: "Despite the animation, all damage is connected in **one single bundle**." This means:
- The total damage (e.g., 380% at Lv10) is calculated as ONE damage value
- It is delivered as ONE damage packet
- The visual shows two arrows but mechanically it is ONE hit
- Cannot partially miss (either the whole hit lands or misses)
- Lex Aeterna doubles the ENTIRE bundle (not just one "hit")

**Implementation:** Single `calculatePhysicalDamage()` call at the total multiplier, one `skill:effect_damage` broadcast with `hits: 1`.

---

### 1.5 Arrow Shower (ID 304, rA: AC_SHOWER)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged, Ground AoE | All |
| Max Level | 10 | All |
| Prerequisites | Double Strafe Lv 5 | All |
| SP Cost | 15 (all levels) | All |
| Cast Time | 0 (instant) | rAthena pre-re |
| After-Cast Delay | 1000ms | iRO Wiki Classic |
| Cooldown | 0 | rAthena pre-re |
| Range | 9 cells base + Vulture's Eye bonus | rAthena |
| AoE | **5x5 cells** (all levels, pre-renewal) | RateMyServer pre-re, iRO Classic |
| Knockback | 2 cells, direction: away from ground target center | iRO Wiki, rAthena |
| Element | Weapon/Arrow element | rAthena |
| Weapon Required | Bow | All |
| Arrow Consumed | 1 per cast | All |
| Target | Ground | All |

**IMPORTANT -- Pre-renewal vs Renewal AoE:**
- Pre-renewal: 5x5 at ALL levels (constant)
- Renewal: 3x3 at Lv1-5, 5x5 at Lv6-10

**Damage Table (Pre-Renewal):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 80 | 85 | 90 | 95 | 100 | 105 | 110 | 115 | 120 | 125 |

**Formulas:**
```
ATK% = (75 + 5 * SkillLv)
```
Confirmed by rAthena source: `100 - 25 + 5 * skill_level`

**Knockback Direction:** Enemies are pushed 2 cells away from the ground target position (NOT from the caster). If an enemy is standing on the exact target cell, they are pushed in a default direction (typically westward in rAthena).

**Special Properties:**
- Can hit cloaked/hidden targets
- Can displace traps (pushes traps 2 cells from AoE center)
- Boss monsters immune to knockback (damage still applies)
- Each enemy in AoE takes independent damage (no split)

---

### 1.6 Arrow Crafting (ID 305, rA: AC_MAKINGARROW)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self | All |
| Max Level | 1 | All |
| SP Cost | 10 | iRO Wiki, rAthena |
| Cast Time | 0 | rAthena |
| Acquisition | Quest Skill (Job Lv 30+) | All |
| Weight Restriction | Cannot use if >= 50% weight capacity | rAthena |

**Quest Requirements (Archer Platinum Skill Quest):**
- NPC: Roberto (moc_ruins 118/99)
- Items: 7 Mushroom Spore, 1 Red Potion, 20 Resin, 13 Trunk, 41 Pointed Scale
- Job Level: 30+

Converts inventory items into arrows. Opens a selection UI where the player chooses which inventory item to convert. See section 7 for complete recipe list.

---

### 1.7 Arrow Repel (ID 306, rA: AC_CHARGEARROW)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged | All |
| Max Level | 1 | All |
| SP Cost | 15 | All |
| Cast Time | 1500ms (1.5s) | rAthena pre-re |
| Cast Interruptible | Not interruptible (iRO Classic) / Yes (rAthena) | Conflicting |
| After-Cast Delay | 0 | rAthena pre-re |
| Cooldown | 0 | rAthena pre-re |
| Range | 9 cells base + Vulture's Eye bonus | rAthena |
| Damage | 150% ATK | All |
| Knockback | 6 cells away from caster | iRO Classic, RateMyServer |
| Element | Weapon/Arrow element | rAthena |
| Weapon Required | Bow | All |
| Arrow Consumed | 1 per cast | All |
| Acquisition | Quest Skill (Job Lv 35+) | All |

**Quest Requirements (Archer Platinum Skill Quest):**
- NPC: Jason (payon 103/63)
- Items: 36 Banana Juice, 10 Bill of Bird, 2 Emerald, 10 Tentacle, 3 Yoyo Tail
- Job Level: 35+

Standard physical damage pipeline at 150% ATK. Boss monsters immune to knockback (damage still applies). The knockback direction is away from the caster (not the target position). Arrow Repel is blocked by Pneuma (ranged physical).

---

## 2. Hunter Skills (IDs 900-917)

### Class Overview

The Hunter is the 2nd class specialization of Archer, defined by two unique systems:
1. **Trap System** -- Ground-placed objects that trigger on enemy contact. 12 trap-related skills.
2. **Falcon System** -- Companion falcon for Blitz Beat, Detect, and Spring Trap. 2 falcon skills + 2 passives.

**Inherited Skills:** All 7 Archer skills (IDs 300-306) carry over.

### 2.1 Blitz Beat (ID 900, rA: HT_BLITZBEAT)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, MISC damage | All |
| Max Level | 5 | All |
| Target | Single (3x3 splash) | All |
| Element | **Neutral** (always, ignores weapon/arrow element) | All |
| Range | 5 cells + Vulture's Eye level | rAthena |
| Cast Time | 1500ms base (variable, reduced by DEX) | rAthena pre-re |
| After-Cast Delay | 1000ms | rAthena |
| Cooldown | 0 | rAthena |
| Requires | Falcon companion | All |
| Prerequisites | Falconry Mastery (916) Lv 1 | All |

**SP Cost / Hit Count Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 10 | 13 | 16 | 19 | 22 |
| Hits | 1 | 2 | 3 | 4 | 5 |

**SP Cost Formula:** `7 + 3 * SkillLv`

**Damage Formula (Pre-Renewal, rAthena `battle.cpp`):**
```c
// rAthena source (pre-renewal branch):
md.damage = (sstatus->dex/10 + sstatus->int_/2 + skill*3 + 40) * 2;
// where skill = Steel Crow level
```

Expanded:
```
PerHitDamage = (floor(DEX/10) + floor(INT/2) + SteelCrowLv*3 + 40) * 2
TotalDamage = PerHitDamage * NumberOfHits
```

Equivalent simplified form:
```
PerHitDamage = 80 + 2*floor(DEX/10) + 2*floor(INT/2) + SteelCrowLv*6
```

**Example (INT=70, DEX=99, Steel Crow 10, Blitz Beat Lv5):**
```
PerHit = (floor(99/10) + floor(70/2) + 10*3 + 40) * 2
       = (9 + 35 + 30 + 40) * 2
       = 114 * 2 = 228 per hit
Total  = 228 * 5 = 1,140 damage
```

**MISC Damage Properties:**
- Ignores DEF and MDEF completely
- Ignores FLEE (always hits, cannot miss)
- Ignores card race/element/size modifiers
- Ignores size penalty
- Always Neutral element (but element table still applies to target's element)
- NOT affected by ATK, MATK, weapon element, or endow skills
- Bypasses Guard, Parry, and Weapon Blocking

**Manual Blitz Beat vs Auto-Blitz Beat:**

| Property | Manual | Auto-Blitz |
|----------|--------|------------|
| SP Cost | 10-22 (by level) | 0 (free) |
| Hits | 1-5 (skill level) | 1-5 (job level formula) |
| Targeting | Player selects target | Current auto-attack target |
| 3x3 Splash | **Full** damage to all in AoE | Damage **SPLIT** among targets |
| Cast Time | 1500ms base | None (instant) |
| After-Cast Delay | 1000ms | None |
| Trigger | Player activates | LUK/3 % on normal bow attack |

See section 4 for the full Falcon System details.

---

### 2.2 Steel Crow (ID 901, rA: HT_STEELCROW)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All |
| Max Level | 10 | All |
| Prerequisites | Blitz Beat (900) Lv **5** | rAthena |

**Effect:** +6 Falcon ATK per level, applied in the Blitz Beat damage formula.

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Falcon +ATK | +6 | +12 | +18 | +24 | +30 | +36 | +42 | +48 | +54 | +60 |

Steel Crow is NOT added to `getPassiveSkillBonuses()`. It is read directly in the Blitz Beat/Auto-Blitz damage calculation:
```js
const steelCrowLv = learned[901] || 0;
// Used inside: (floor(DEX/10) + floor(INT/2) + steelCrowLv*3 + 40) * 2
```

---

### 2.3 Detect (ID 902, rA: HT_DETECTING)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Utility | All |
| Max Level | 4 | All |
| Target | Ground | All |
| AoE | 7x7 cells (constant, all levels) | rAthena `SplashArea: 3` |
| SP Cost | 8 (flat, all levels) | rAthena pre-re |
| Cast Time | 0 | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| Requires | Falcon companion | All |
| Prerequisites | Improve Concentration (302) Lv 1, Falconry Mastery (916) Lv 1 | rAthena |

**Range by Level:**

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| Range (cells) | 3 | 5 | 7 | 9 |

**Mechanics:**
- Reveals Hiding, Cloaking, and Chase Walk
- Also reveals invisible enemy traps (PvP/WoE)
- Detection area is always 7x7 cells centered on the targeted ground position
- Range to target position increases with level (3-9 cells)
- One-time reveal (not persistent)

---

### 2.4 Ankle Snare (ID 903, rA: HT_ANKLESNARE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Control | All |
| Max Level | 5 | All |
| Target | Ground | All |
| AoE | 1x1 (trigger cell only) | All |
| SP Cost | 12 (all levels) | All |
| Cast Time | 0 | All |
| Required Item | 1 Trap | rAthena |
| Prerequisites | Skid Trap (908) Lv 1 | rAthena |

See section 6 for full Ankle Snare special mechanics.

**Duration Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Base Snare Duration (s) | 4 | 8 | 12 | 16 | 20 |
| Trap Lifetime (s) | 250 | 200 | 150 | 100 | 50 |

**Formulas:**
```
Base snare duration = 4 * SkillLv seconds
Trap lifetime = (6 - SkillLv) * 50 seconds
```

---

### 2.5 Land Mine (ID 904, rA: HT_LANDMINE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Offensive | All |
| Max Level | 5 | All |
| Target | Ground | All |
| Element | Earth | All |
| AoE | 1x1 (trigger cell only) | All |
| SP Cost | 10 (all levels) | All |
| Required Item | 1 Trap | rAthena |
| Prerequisites | None | rAthena |

**Pre-Renewal Damage Formula (iRO Wiki Classic):**
```
Damage = SkillLv * (75 + DEX) * (1 + INT / 100)
```

Note: The renewal formula is different: `DEX * (3 + BaseLv/100) * (1 + INT/35) * SkillLv + TrapResearch*40`. The pre-renewal formula does NOT include Base Level scaling or Trap Research bonus.

**Stun Mechanic:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Stun Chance | 35% | 40% | 45% | 50% | 55% |
| Stun Duration | 5s | 5s | 5s | 5s | 5s |
| Trap Lifetime (s) | 200 | 160 | 120 | 80 | 40 |

**Stun Chance Dispute:**
- RateMyServer: `30 + 5 * SkillLv` (35-55%)
- rAthena source: `sc_start(SC_STUN, 10)` which could mean 10% flat base
- Boss monsters immune to stun (damage still applies)
- Damage is MISC type (ignores DEF, MDEF, FLEE, cards, size penalty)
- Earth element modifier applies to target

---

### 2.6 Remove Trap (ID 905, rA: HT_REMOVETRAP)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Utility | All |
| Max Level | 1 | All |
| Target | Trap on ground | All |
| Range | 2 cells | iRO Wiki |
| SP Cost | 5 | All |
| Cast Time | 0 | All |
| Prerequisites | Land Mine (904) Lv 1 | rAthena |

**Mechanics:**
- Can only remove YOUR OWN traps (not enemy traps)
- Returns 1 Trap item to inventory
- Cannot remove traps that are already triggered/activated
- Short range -- must stand adjacent to the trap
- Hunter class cannot remove Rogue traps and vice-versa

---

### 2.7 Shockwave Trap (ID 906, rA: HT_SHOCKWAVE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Debuff | All |
| Max Level | 5 | All |
| Target | Ground | All |
| AoE | 3x3 cells | rAthena |
| SP Cost | 45 (all levels) | All |
| Required Item | **2 Traps** | rAthena |
| Prerequisites | Ankle Snare (903) Lv 1 | rAthena |

**SP Drain Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Drain % | 20% | 35% | 50% | 65% | 80% |
| Trap Lifetime (s) | 200 | 160 | 120 | 80 | 40 |

**Formulas:**
```
SP drained = floor(TargetCurrentSP * DrainPercent / 100)
DrainPercent = 5 + 15 * SkillLv
```

**Mechanics:**
- Does NOT affect Boss monsters (no SP drain)
- Does NOT deal HP damage
- Hits only the target that activated the trap (despite 3x3 AoE field)
- Only affects targets with SP (monsters with 0 SP unaffected)
- Consumes 2 Trap items per placement

---

### 2.8 Claymore Trap (ID 907, rA: HT_CLAYMORETRAP)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Offensive | All |
| Max Level | 5 | All |
| Target | Ground | All |
| Element | **Fire** | All |
| AoE | **5x5 cells** | All |
| SP Cost | 15 (all levels) | All |
| Required Item | **2 Traps** | rAthena |
| Prerequisites | Shockwave Trap (906) Lv 1, Blast Mine (912) Lv 1 | rAthena |

**Pre-Renewal Damage Formula:**
```
Damage = SkillLv * (75 + floor(DEX / 2)) * (1 + INT / 100)
```

Renewal formula (different): `DEX * (3 + BaseLv/85) * (1.1 + INT/35) * SkillLv + TrapResearch*40`

**Trap Lifetime (increases with level, unusual):**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Lifetime (s) | 20 | 40 | 60 | 80 | 100 |

**Formula:** `Lifetime = 20 * SkillLv seconds`

**Mechanics:**
- Largest AoE trap (5x5)
- Highest single-trap damage
- Duration INCREASES with level (opposite of most traps)
- Damage is MISC type (ignores DEF, FLEE, cards, size penalties)
- Fire element modifier applies to target
- Bypasses Guard, Parry, and Weapon Blocking

---

### 2.9 Skid Trap (ID 908, rA: HT_SKIDTRAP)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Utility | All |
| Max Level | 5 | All |
| Target | Ground | All |
| AoE | 1x1 (trigger cell) | All |
| SP Cost | 10 (all levels) | All |
| Required Item | 1 Trap | rAthena |
| Prerequisites | **None** | rAthena |

**Knockback Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Knockback (cells) | 6 | 7 | 8 | 9 | 10 |
| Trap Lifetime (s) | 300 | 240 | 180 | 120 | 60 |

**Formulas:**
```
Knockback = 5 + SkillLv cells
Trap Lifetime = (6 - SkillLv) * 60 seconds
```

**Mechanics:**
- Does NOT deal damage (knockback only)
- Knockback direction: pushes in the direction the trap was facing when placed
- Boss monsters immune to knockback effect (still triggers and consumes trap)
- rAthena: `mob_unlocktarget()` -- monster loses its target after knockback (deaggros)

---

### 2.10 Sandman (ID 909, rA: HT_SANDMAN)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Status | All |
| Max Level | 5 | All |
| Target | Ground | All |
| AoE | **5x5 cells** | rAthena `SplashArea: 2` |
| SP Cost | 12 (all levels) | All |
| Required Item | 1 Trap | rAthena |
| Prerequisites | Flasher (910) Lv 1 | rAthena |

**Sleep Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Sleep Chance | 50% | 60% | 70% | 80% | 90% |
| Sleep Duration (s) | 30 | 30 | 30 | 30 | 30 |
| Trap Lifetime (s) | 150 | 120 | 90 | 60 | 30 |

**Formulas:**
```
Sleep Chance = 40 + 10 * SkillLv (%)
Sleep Duration = 30 seconds (fixed, all levels)
```

**Mechanics:**
- Sleep is breakable by damage (any damage wakes the target)
- Boss monsters immune to sleep (still triggers and consumes trap)
- All targets in 5x5 AoE are checked independently for sleep
- Does NOT deal HP damage

---

### 2.11 Flasher (ID 910, rA: HT_FLASHER)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Status | All |
| Max Level | 5 | All |
| Target | Ground | All |
| AoE | Single target (trigger cell only) | rAthena (no SplashArea) |
| SP Cost | 12 (all levels) | All |
| Required Item | **1 Trap** | rAthena confirmed |
| Prerequisites | Skid Trap (908) Lv 1 | rAthena |

**Blind Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Blind Duration (s) | 30 | 30 | 30 | 30 | 30 |
| Trap Lifetime (s) | 150 | 120 | 90 | 60 | 30 |

**Formulas:**
```
Blind Duration = 30 seconds (fixed, all levels)
Blind Chance = 100% base (resistance applied by status system via VIT+INT)
Level only affects trap ground lifetime, not blind chance or duration
```

**Mechanics:**
- Blind reduces HIT by 25% and FLEE by 25%
- Boss monsters immune to blind
- Hits only the target that activated the trap (no AoE)
- rAthena confirmed: `sc_start(SC_BLIND, 100)` -- 100% base, VIT+INT resistance internally
- Consumes 1 Trap (NOT 2 -- this was a common documentation error)

---

### 2.12 Freezing Trap (ID 911, rA: HT_FREEZINGTRAP)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Offensive + Status | All |
| Max Level | 5 | All |
| Target | Ground | All |
| Element | **Water** | All |
| AoE | 3x3 cells | rAthena |
| SP Cost | 10 (all levels) | All |
| Required Item | **1 Trap** | rAthena confirmed |
| Prerequisites | Flasher (910) Lv 1 | rAthena |

**CRITICAL -- Damage Type Dispute:**

There are two conflicting damage formulas depending on source:

**rAthena skill_db.yml:** `Type: Weapon`, `IgnoreAtkCard: true`
- Damage = `(25 + 25 * SkillLv)%` of ATK
- Uses weapon-type damage pipeline (affected by DEF, can miss)
- Cards are ignored (IgnoreAtkCard flag)

**iRO Wiki Classic / Community standard:**
- Damage = `SkillLv * (DEX + 75) * (1 + INT / 100)` (same as Land Mine)
- MISC type (ignores DEF, MDEF, FLEE)

For a pre-renewal classic implementation, the MISC formula is more commonly cited. However, rAthena source treats it as Weapon type. The implementation should choose one approach and be consistent.

**Freeze Mechanic:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Freeze Duration (s) | 3 | 6 | 9 | 12 | 15 |
| Trap Lifetime (s) | 150 | 120 | 90 | 60 | 30 |

**Formulas:**
```
Freeze Duration = 3 * SkillLv seconds
Freeze Chance = 100% base (MDEF resistance applied internally)
```

**Mechanics:**
- Frozen targets become Water Lv1 element (vulnerable to Wind attacks)
- Boss monsters immune to freeze (damage still applies)
- Freeze is breakable by damage
- Water element modifier applies to damage
- Consumes 1 Trap (NOT 2)

---

### 2.13 Blast Mine (ID 912, rA: HT_BLASTMINE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Offensive | All |
| Max Level | 5 | All |
| Target | Ground | All |
| Element | **Wind** | All |
| AoE | 3x3 cells | rAthena |
| SP Cost | 10 (all levels) | All |
| Required Item | 1 Trap | rAthena |
| Prerequisites | Land Mine (904) Lv 1, Sandman (909) Lv 1, Freezing Trap (911) Lv 1 | rAthena |

**Pre-Renewal Damage Formula:**
```
Damage = SkillLv * (50 + floor(DEX / 2)) * (1 + INT / 100)
```

**Auto-Detonation Timer:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Auto-Detonate (s) | 25 | 20 | 15 | 10 | 5 |

**Formula:** `AutoDetonateTimer = 30 - 5 * SkillLv seconds`

**Mechanics:**
- AUTO-DETONATES after timer expires (even with no target)
- Can also trigger by enemy stepping on it before timer
- Auto-detonation damages all enemies in 3x3 AoE
- Timer starts when trap is placed
- Damage is MISC type
- Wind element modifier applies

---

### 2.14 Spring Trap (ID 913, rA: HT_SPRINGTRAP)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Utility | All |
| Max Level | 5 | All |
| Target | Trap on ground | All |
| SP Cost | 10 (all levels) | All |
| Requires | Falcon companion | All |
| Prerequisites | Remove Trap (905) Lv 1, Falconry Mastery (916) Lv 1 | rAthena |

**Range by Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Range (cells) | 4 | 5 | 6 | 7 | 8 |

**Mechanics:**
- Can target ANY visible trap (own or enemy)
- Does NOT return Trap item to inventory (unlike Remove Trap)
- Requires falcon companion
- Can remove Hunter-class traps only (not Rogue traps)
- Useful for WoE/PvP counter-trapping
- No cast time, no after-cast delay

---

### 2.15 Talkie Box (ID 914, rA: HT_TALKIEBOX)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Trap, Cosmetic | All |
| Max Level | 1 | All |
| Target | Ground | All |
| SP Cost | 1 | All |
| Required Item | 1 Trap | All |
| Prerequisites | Shockwave Trap (906) Lv 1, Remove Trap (905) Lv 1 | rAthena |

**Mechanics:**
- Player inputs a text message at cast time
- Trap displays the message as a chat bubble when any player steps on it
- Trap lasts 600 seconds (10 minutes)
- Purely cosmetic -- no damage, no status effects
- Requires client text input UI

---

### 2.16 Beast Bane (ID 915, rA: HT_BEASTBANE)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All |
| Max Level | 10 | All |
| Prerequisites | None | rAthena |

**Effect:** +4 flat ATK per level vs Brute and Insect race monsters (ignores DEF).

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

**Implementation:** Same pattern as Acolyte's Demon Bane. Added to `getPassiveSkillBonuses()` as race-specific ATK:
```js
raceATK.brute += SkillLv * 4;
raceATK.insect += SkillLv * 4;
```
Applied at step 11b of the physical damage pipeline (after card mods, before element modifier). This bonus bypasses hard DEF and soft DEF -- it is flat ATK added AFTER all DEF reductions.

---

### 2.17 Falconry Mastery (ID 916, rA: HT_FALCON)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All |
| Max Level | 1 | All |
| Prerequisites | Beast Bane (915) Lv 1 | rAthena |

**Effect:** Enables the falcon companion. Gate skill for: Blitz Beat, Detect, Spring Trap, Auto-Blitz Beat.

**Falcon Rules:**
- Rented from Hunter Guild NPC for 2,500 Zeny (some sources say 100z)
- Cannot have falcon and Pushcart at the same time (mutually exclusive)
- Server tracks `player.hasFalcon = true/false`
- Falcon persists through zone transitions (same as Pushcart)
- Falcon is NOT lost on death in pre-renewal

---

### 2.18 Phantasmic Arrow (ID 917, rA: HT_PHANTASMIC)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged | All |
| Max Level | 1 | All |
| Target | Single enemy | All |
| Element | Arrow element if equipped, otherwise Neutral | rAthena |
| Range | 9 cells | rAthena |
| SP Cost | 10 | rAthena, RateMyServer |
| Cast Time | 0 | rAthena |
| After-Cast Delay | 0 | rAthena |
| Damage | 150% ATK | All |
| Knockback | 3 cells away from caster | iRO Wiki |
| Weapon Required | Bow | rAthena |
| Arrow Consumed | **None** (creates a phantom arrow) | All |
| Acquisition | Quest Skill (Job Lv 40+) | All |

**Quest Requirements (Hunter Platinum Skill Quest):**
- NPCs: Arpesto and Reidin Corse (north end of Archer Village)
- Items: 5 Cursed Rubies, 5 Harpy Feathers, 30 Pet Foods

**Mechanics:**
- Does NOT consume arrows (the key feature of this skill)
- Uses arrow element if arrows are equipped, otherwise defaults to Neutral
- Standard physical skill (affected by DEF, cards, size penalty, FLEE, etc.)
- NOT MISC type -- regular physical damage pipeline at 150%
- Boss monsters immune to knockback (damage still applies)
- Blocked by Pneuma (ranged physical)

---

## 3. Arrow System

### 3.1 Arrow Types

There are 21 arrow types in pre-renewal RO Classic:

| Arrow | Item ID | Element | Special Effect |
|-------|---------|---------|----------------|
| Arrow | 1750 | Neutral | None |
| Silver Arrow | 1751 | Holy | None |
| Fire Arrow | 1752 | Fire | None |
| Steel Arrow | 1753 | Neutral | +ATK |
| Crystal Arrow | 1754 | Water | None |
| Wind Arrow | 1755 | Wind | None |
| Stone Arrow | 1756 | Earth | None |
| Immaterial Arrow | 1757 | Ghost | None |
| Stun Arrow | 1758 | Neutral | Stun chance |
| Frozen Arrow | 1759 | Water | Freeze chance |
| Flash Arrow | 1760 | Neutral | Blind chance |
| Cursed Arrow | 1761 | Neutral | Curse chance |
| Rusty Arrow | 1762 | Neutral | Poison chance |
| Poison Arrow | 1763 | Poison | None |
| Sharp Arrow | 1764 | Neutral | +20 CRIT |
| Oridecon Arrow | 1765 | Neutral | Highest ATK |
| Arrow of Counter Evil | 1766 | Holy | None |
| Shadow Arrow | 1767 | Shadow | None |
| Sleep Arrow | 1768 | Neutral | Sleep chance |
| Mute Arrow | 1769 | Neutral | Silence chance |
| Iron Arrow | 1770 | Neutral | None |
| Holy Arrow | 1772 | Holy | None |

### 3.2 Arrow Element Priority

When determining attack element for ranged attacks:
```
1. Endow element (highest priority — Aspersio, Flameguard, etc.)
2. Arrow element (if non-Neutral arrow equipped)
3. Weapon element (base weapon element)
4. Neutral (default)
```

The function `recalcEffectiveWeaponElement()` handles this priority chain.

### 3.3 Arrow Consumption

- **Auto-attacks:** 1 arrow consumed per hit (per attack cycle)
- **Skills:** 1 arrow consumed per skill cast:
  - Double Strafe (303)
  - Arrow Shower (304)
  - Arrow Repel (306)
  - Musical Strike (1506) (Bard)
  - Slinging Arrow (1521) (Dancer)
- **NOT consumed by:** Phantasmic Arrow (917) -- creates its own

### 3.4 Status Arrow Proc Rates

Status arrows have a base chance to inflict their status on the target per hit:

| Arrow Type | Status | Base Chance | Boss Immune |
|-----------|--------|-------------|-------------|
| Stun Arrow | Stun | 10% | Yes |
| Frozen Arrow | Freeze | 10% | Yes |
| Flash Arrow | Blind | 10% | Yes |
| Cursed Arrow | Curse | 10% | Yes |
| Sleep Arrow | Sleep | 10% | Yes |
| Mute Arrow | Silence | 10% | Yes |
| Rusty Arrow | Poison | 20% | Yes |

### 3.5 Quiver Items

Quiver items are consumable items that instantly add 500 arrows of a specific type to inventory:

| Quiver | Produces | Qty |
|--------|----------|-----|
| Arrow Quiver | Arrow (1750) | 500 |
| Silver Arrow Quiver | Silver Arrow (1751) | 500 |
| Fire Arrow Quiver | Fire Arrow (1752) | 500 |
| Steel Arrow Quiver | Steel Arrow (1753) | 500 |
| Crystal Arrow Quiver | Crystal Arrow (1754) | 500 |
| Stone Arrow Quiver | Stone Arrow (1756) | 500 |
| Wind Arrow Quiver | Wind Arrow (1755) | 500 |
| Shadow Arrow Quiver | Shadow Arrow (1767) | 500 |
| Immaterial Arrow Quiver | Immaterial Arrow (1757) | 500 |
| Rusty Arrow Quiver | Rusty Arrow (1762) | 500 |
| Oridecon Arrow Quiver | Oridecon Arrow (1765) | 500 |
| Sharp Arrow Quiver | Sharp Arrow (1764) | 500 |
| Holy Arrow Quiver | Holy Arrow (1772) | 500 |

---

## 4. Falcon System

### 4.1 Falcon State

The falcon is a companion entity tracked on the player object:
```js
player.hasFalcon = false; // Set true when falcon rented from Hunter Guild NPC
```

**Falcon Rules:**
- Requires Falconry Mastery Lv 1 (skill 916)
- Rented from Hunter Guild NPC for 2,500 Zeny
- Cannot have falcon and Pushcart simultaneously
- Required for: Blitz Beat, Detect, Spring Trap, Auto-Blitz Beat
- Persists through zone transitions
- NOT lost on death in pre-renewal
- Visual: falcon perches on the Hunter's shoulder (cosmetic, no hitbox)

### 4.2 Blitz Beat Damage Formula (Canonical rAthena)

```
PerHitDamage = (floor(DEX/10) + floor(INT/2) + SteelCrowLv*3 + 40) * 2
TotalDamage = PerHitDamage * NumberOfHits
```

The equivalent simplified form:
```
PerHitDamage = 80 + 2*floor(DEX/10) + 2*floor(INT/2) + SteelCrowLv*6
```

**Damage scaling examples (Steel Crow 10, Blitz Beat Lv5):**

| INT | DEX | Per Hit | 5 Hits Total |
|-----|-----|---------|--------------|
| 1 | 99 | (9+0+30+40)*2 = 158 | 790 |
| 30 | 99 | (9+15+30+40)*2 = 188 | 940 |
| 50 | 99 | (9+25+30+40)*2 = 208 | 1,040 |
| 70 | 99 | (9+35+30+40)*2 = 228 | 1,140 |
| 99 | 99 | (9+49+30+40)*2 = 256 | 1,280 |

INT is the most impactful stat for Blitz Beat damage. DEX has minimal impact (only floor(DEX/10) contributes).

### 4.3 Auto-Blitz Beat

Auto-Blitz triggers as a passive proc during normal bow auto-attacks.

**Trigger Chance:**
```
Chance = floor(LUK / 3) %
```
For every 3 LUK, 1% chance of Auto-Blitz on each normal attack. At LUK 99: `floor(99/3) = 33%` chance.

**Hit Count Formula (rAthena `skill.cpp`):**
```c
// rAthena source:
(sd->status.job_level + 9) / 10
```
Equivalent: `floor((JobLevel + 9) / 10)`

| Job Level | Auto-Blitz Hits |
|-----------|-----------------|
| 1-9 | 1 |
| 10-19 | 2 |
| 20-29 | 3 |
| 30-39 | 4 |
| 40-50 | 5 |

Capped at learned Blitz Beat level (e.g., Blitz Beat Lv3 caps auto-blitz at 3 hits regardless of job level).

**Auto-Blitz vs Manual differences:**
- Auto-Blitz has **no SP cost** (free)
- Auto-Blitz has **no cast time** (instant)
- Auto-Blitz has **no after-cast delay**
- Auto-Blitz damage is **SPLIT** among all targets in 3x3 AoE:
  ```
  PerTargetDamage = floor(TotalDamage / NumberOfTargetsInAoE)
  ```
- Manual Blitz Beat deals **full damage** to each target in AoE
- Auto-Blitz **can trigger even on MISSED auto-attacks** (rAthena: check is in `skill_additional_effect()` which proceeds for `ATK_BLOCK`)
- Auto-Blitz **triggers independent of skill cooldowns/delays**

### 4.4 Falcon Assault (Sniper -- transcendent, deferred)

Sniper gets Falcon Assault (rA: SN_FALCONASSAULT), which deals much higher MISC damage using a different formula:
```
Damage = (100 + BlitzBeatLv * 20)% * (PATK + 60 + SteelCrowBonus)
```
Not implemented in current scope (transcendent class).

---

## 5. Trap System

### 5.1 MISC Damage Type

All trap damage skills (except Freezing Trap in rAthena) use MISC damage, a third damage category alongside Physical and Magical:

| Property | MISC | Physical | Magical |
|----------|------|----------|---------|
| Ignores DEF | YES | No | N/A |
| Ignores MDEF | YES | N/A | No |
| Ignores FLEE | YES | No | Yes |
| Ignores Cards | YES | No | No |
| Ignores Size Penalty | YES | No | Yes |
| Element Table | YES | YES | YES |
| HIT/FLEE Check | No | Yes | No |
| Can Miss | No | Yes | No |
| Critical | No | Yes | No |
| Bypasses Guard/Parry | YES | No | N/A |

### 5.2 Trap Damage Formulas (Pre-Renewal, Consolidated)

All MISC-type trap damage uses this general formula:
```
BaseDamage = SkillLv * (BaseValue) * (1 + INT / 100)
FinalDamage = floor(BaseDamage * ElementModifier / 100) +/- 10% variance
```

| Trap | BaseValue | Element | AoE | Notes |
|------|-----------|---------|-----|-------|
| Land Mine | DEX + 75 (full DEX) | Earth | 1x1 | +Stun |
| Blast Mine | 50 + floor(DEX/2) | Wind | 3x3 | Auto-detonates |
| Claymore Trap | 75 + floor(DEX/2) | Fire | 5x5 | Longest duration |
| Freezing Trap | DEX + 75 (same as Land Mine) | Water | 3x3 | +Freeze (disputed type) |

**Example Calculations (DEX=90, INT=50, SkillLv=5):**
```
Land Mine:  5 * (90+75) * (1+50/100) = 5 * 165 * 1.5 = 1,237 base
Blast Mine: 5 * (50+45) * (1+50/100) = 5 * 95  * 1.5 =   712 base
Claymore:   5 * (75+45) * (1+50/100) = 5 * 120 * 1.5 =   900 base
Freezing:   5 * (90+75) * (1+50/100) = same as Land Mine = 1,237 base
```

### 5.3 Trap Placement Rules

1. **Maximum active traps:** 3 per Hunter (rAthena `skill_max_trap = 3`). 4th placement destroys the oldest.
2. **Minimum spacing:** Cannot place within 2 cells of another trap, player, or monster.
3. **Trap visibility:** Visible to caster and party members. Enemy players see traps only after trigger (PvP/WoE).
4. **Zone transitions:** All traps destroyed when owner changes zones.
5. **Trap item:** Item ID 1065, weight 10, buy price 100z from NPC.

### 5.4 Trap Item Cost

| Trap Skill | Trap Items |
|-----------|-----------|
| Skid Trap (908) | 1 |
| Land Mine (904) | 1 |
| Ankle Snare (903) | 1 |
| Sandman (909) | 1 |
| Blast Mine (912) | 1 |
| Flasher (910) | 1 |
| Freezing Trap (911) | 1 |
| Shockwave Trap (906) | **2** |
| Claymore Trap (907) | **2** |
| Talkie Box (914) | 1 |
| Remove Trap (905) | 0 (returns 1) |
| Spring Trap (913) | 0 |

Note: rAthena source confirmed Flasher and Freezing Trap cost 1 Trap each. Only Shockwave and Claymore cost 2.

### 5.5 Trap Lifecycle

```
Phase 1: PLACEMENT
  Client -> skill:use { skillId, groundX, groundY, groundZ }
  Server validates: class, skill learned, trap items, range, spacing
  Server deducts SP + trap item(s)
  Server creates TrapEntry in activeTraps Map
  Server -> trap:placed { trapId, skillId, x, y, z, element, lifetime }

Phase 2: ACTIVE (waiting for trigger)
  Server checks every 200ms (enemy AI tick):
    For each enemy in same zone as trap:
      If distance(enemy, trap) < triggerRadius (1 cell = 50 UE)
        -> Trigger trap

Phase 3: TRIGGER
  Execute trap effect (damage/status/knockback)
  Broadcast trap:triggered and skill:effect_damage
  Remove trap from activeTraps

Phase 4: EXPIRY/REMOVAL
  1-second interval checks:
    If Blast Mine auto-detonate timer reached -> detonate (AoE damage)
    If natural lifetime expired -> remove trap, optionally return trap item
  Remove Trap -> returns trap item, removes entry
  Spring Trap -> removes entry, no item return
```

### 5.6 Trap Ground Entity State

```js
TrapEntry = {
    trapId: Number,            // Unique auto-increment ID
    ownerId: Number,           // Character ID of hunter who placed it
    ownerName: String,
    skillId: Number,           // 904, 907, etc.
    skillName: String,         // 'land_mine', 'claymore_trap'
    skillLevel: Number,
    x: Number, y: Number, z: Number,  // Ground position (UE units)
    zone: String,
    element: String,           // 'earth', 'fire', 'wind', 'water', 'neutral'
    placedAt: Number,          // Date.now()
    lifetime: Number,          // Milliseconds
    expiresAt: Number,         // placedAt + lifetime
    triggerRadius: Number,     // 50 UE units (1 cell)
    aoeRadius: Number,         // Varies: 0 (1x1), 75 (3x3), 125 (5x5)
    isTriggered: Boolean,
    autoDetonateAt: Number,    // Blast Mine only (0 = never)
    trapItemCost: Number,      // 1 or 2
    // Cached owner stats at placement time:
    ownerDEX: Number,
    ownerINT: Number,
    ownerBaseLv: Number,
    steelCrowLv: Number,
    ownerATK: Number,          // For Freezing Trap if using Weapon formula
}
```

### 5.7 Socket Events (Traps)

| Event | Direction | Payload |
|-------|-----------|---------|
| `trap:placed` | Server -> Zone | `{ trapId, ownerId, skillId, skillName, x, y, z, element, lifetime }` |
| `trap:triggered` | Server -> Zone | `{ trapId, skillName, x, y, z, triggeredById, triggeredByName }` |
| `trap:detonated` | Server -> Zone | `{ trapId, skillName, x, y, z }` |
| `trap:expired` | Server -> Zone | `{ trapId }` |
| `trap:removed` | Server -> Zone | `{ trapId, removedById }` |

Trap damage uses standard `skill:effect_damage` with `hitType: 'misc'`.

---

## 6. Ankle Snare Special Mechanics

Ankle Snare is the most mechanically complex trap skill. Its behavior differs significantly from stun/freeze.

### 6.1 Status Type: `ankle_snare` (NOT `stun`)

rAthena `db/pre-re/status.yml` defines SC_ANKLE:
```yaml
- Status: Ankle
  States:
    NoMove: true        # ONLY NoMove
  Flags:
    NoClearbuff: true
    StopWalking: true
    NoDispell: true
    RemoveOnChangeMap: true
```

**Key difference from stun:**
- Ankle Snare: target CANNOT move, but CAN attack, cast skills, and use items
- Stun: target cannot do ANYTHING (move, attack, cast, use items)

### 6.2 Duration Formulas

**Monsters (normal):**
```
BaseDuration = 4 * SkillLv seconds
AdjustedDuration = max(BaseDuration * (1 - TargetAGI / 200), 30 * (CasterBaseLv + 100) ms)
```

The AGI formula means high-AGI monsters escape faster. The minimum duration is based on the caster's base level, creating a floor that prevents instant escapes.

**Boss Monsters (1/5 duration):**
```
BossDuration = AdjustedDuration / 5
```
rAthena source: `if (status_bl_has_mode(bl, MD_STATUSIMMUNE)) sec /= 5;`

Bosses are NOT fully immune -- they DO get trapped, just for 1/5 the normal duration. This is a significant gameplay mechanic for MVP hunting.

**Players (PvP/WoE):**
```
PlayerDuration = max(5 * SkillLv / (TargetAGI * 0.1), 3) seconds
```
Minimum 3 seconds for players.

### 6.3 Interaction Rules

- Target can still attack enemies in melee range while snared
- Target can cast spells while snared
- Target can use items while snared
- Snare is NOT broken by damage
- Snare is NOT dispellable
- Snare IS removed on map change
- Ankle Snare does NOT deal damage (immobilize only)
- Trapped enemies are valid targets for auto-attack and skills
- PvP: snares trap ALL players, including the caster's own party and the caster themselves

### 6.4 Implementation Notes

Requires a new status type `ankle_snare` distinct from `stun`:
```js
ankle_snare: {
    resistStat: null,            // Always applies (SCSTART_NORATEDEF)
    resistCap: null,
    baseDuration: 20000,
    canKill: false,
    breakOnDamage: false,        // NOT broken by damage
    preventsMovement: true,      // ONLY prevents movement
    preventsCasting: false,      // CAN cast
    preventsAttack: false,       // CAN attack
    preventsItems: false,        // CAN use items
    blocksHPRegen: false,
    blocksSPRegen: false,
    statMods: {}
}
```

---

## 7. Arrow Crafting

### 7.1 Skill Mechanics

- **Skill ID:** 305 (rA: AC_MAKINGARROW)
- **SP Cost:** 10
- **Weight Restriction:** Cannot use if >= 50% weight capacity
- **UI:** Opens a selection UI showing craftable items from inventory

### 7.2 Complete Recipe List

Recipes are stored in `server/src/ro_arrow_crafting.js`. Format: consume 1 of the source item to produce the specified quantity of arrows.

#### Basic Arrows (1750)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Jellopy | 909 | 4 |
| Tree Root | 902 | 7 |
| Trunk | 1019 | 40 |
| Resin | 907 | 3 |
| Feather | 940 | 2 |
| Claw of Wolves | 916 | 5 |
| Tooth of Bat | 7001 | 2 |
| Log (Wooden Block) | 1020 | 20 |
| Shell | 935 | 10 |
| Worm Peeling | 939 | 3 |
| Insect Feeler | 914 | 5 |
| Chonchon Doll | 904 | 3 |
| Solid Trunk | 7063 | 15 |

#### Iron Arrows (1770) / Steel Arrows (1753)

| Source Item | Item ID | Arrow Type | Quantity |
|-------------|---------|------------|----------|
| Iron | 998 | Iron Arrow | 100 |
| Phracon | 1010 | Iron Arrow | 50 |
| Steel | 999 | Steel Arrow | 100 |
| Emveretarcon | 1011 | Steel Arrow | 50 |
| Elunium | 985 | Steel Arrow | 1,000 |

#### Fire Arrows (1752)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Red Blood | 990 | 600 |
| Flame Heart | 994 | 1,800 |
| Burning Heart | 7097 | 30 |

#### Crystal Arrows (Water, 1754)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Crystal Blue | 991 | 150 |
| Mystic Frozen | 995 | 450 |

#### Wind Arrows (1755)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Wind of Verdure | 992 | 150 |
| Rough Wind | 996 | 450 |

#### Stone Arrows (Earth, 1756)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Green Live | 993 | 150 |
| Great Nature | 997 | 450 |

#### Holy Arrows (1772) / Silver Arrows (1751) / Counter Evil (1766)

| Source Item | Item ID | Arrow Type | Quantity |
|-------------|---------|------------|----------|
| Valhalla's Flower | 7510 | Holy Arrow | 600 |
| Holy Water | 912 | Silver Arrow | 3 |
| Rosary in Mouth | 7340 | Counter Evil Arrow | 150 |

#### Ghost Arrows (Immaterial, 1757)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Emperium | 714 | 600 |

#### Shadow Arrows (1767)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Dark Crystal Fragment | 7015 | 100 |

#### Rusty Arrows (Poison, 1762)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Venom Canine | 937 | 30 |
| Poison Spore | 936 | 10 |
| Bee Sting | 7033 | 100 |

#### Oridecon Arrows (1765)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Oridecon | 984 | 250 |

#### Status Arrows

| Source Item | Item ID | Arrow Type | Quantity |
|-------------|---------|------------|----------|
| Phracon Fragment | 7014 | Stun Arrow (1758) | 150 |
| Snowy Fragment | 7066 | Frozen Arrow (1759) | 150 |
| Light Granule | 7031 | Flash Arrow (1760) | 100 |
| Dark Granule | 7030 | Cursed Arrow (1761) | 100 |
| Moth Dust | 7018 | Sleep Arrow (1768) | 150 |
| Mute Fragment | 7032 | Mute Arrow (1769) | 100 |

#### Sharp Arrows (1764, +20 CRIT)

| Source Item | Item ID | Quantity |
|-------------|---------|----------|
| Horrendous Mouth | 947 | 100 |
| Talon of Griffon | 7053 | 150 |

### 7.3 Additional Recipes (from iRO Wiki, not in current data file)

The following recipes are documented in iRO Wiki Classic but may not be in the current `ro_arrow_crafting.js`. They should be verified and added:

| Source Item | Arrow Type | Quantity | Notes |
|-------------|-----------|----------|-------|
| Cactus Needle | Arrow (1750) | 50 | From Muka |
| Porcupine Quill | Arrow (1750) | 70 | From Porcupine |
| Needle of Alarm | Arrow (1750) | 100 | From Alarm |
| Barren Trunk | Arrow (1750) | 20 | From Elder Willow |
| Fine-grained Trunk | Arrow (1750) | 20 | From Wooden Golem |
| Wooden Mail | Arrow (1750) | 700 | Armor |
| Scell | Steel Arrow (1753) | 8 | From various |
| Orc Claw | Steel Arrow (1753) | 10 | From Orc Warrior |
| Orc Fang | Stone Arrow (1756) | 1 | From Orc |
| Mole Claw | Stone Arrow (1756) | 2 | From Mole |
| Yellow Gemstone | Stone Arrow (1756) | 10 | From shops |
| Mantis Scythe | Sharp Arrow (1764) | 1 | From Mantis |
| Fang | Sharp Arrow (1764) | 2 | From various |
| Matchstick | Fire Arrow (1752) | 3,000 | Highest quantity recipe |

---

## 8. Skill Trees and Prerequisites

### 8.1 Archer Skill Tree

```
[Owl's Eye (300)]
     |
     v (Lv 3)
[Vulture's Eye (301)]
     |
     v (Lv 1)
[Improve Concentration (302)]
     |
     v (n/a -- no prereq chain)
[Double Strafe (303)] -----> (Lv 5) [Arrow Shower (304)]

Quest Skills (no tree position):
[Arrow Crafting (305)] -- Job Lv 30 quest
[Arrow Repel (306)] -- Job Lv 35 quest
```

### 8.2 Hunter Skill Tree

```
                        [Beast Bane (915)]
                              |
                              v (Lv 1)
                    [Falconry Mastery (916)]
                         /        \
                        v          v
              [Blitz Beat (900)]  [Detect (902)] -- also needs IC (302) Lv 1
                    |
                    v (Lv 5)
             [Steel Crow (901)]


[Skid Trap (908)]        [Land Mine (904)]
   /      \                     |
  v(L1)   v(L1)               v (Lv 1)
[Ankle    [Flasher          [Remove Trap (905)]
Snare      (910)]               /         \
(903)]    /    \               v            v
  |      v(L1) v(L1)   [Spring Trap    [Talkie Box
  v    [Sandman] [Freezing   (913)]       (914)]
[Shock-  (909)]   Trap     needs FM     needs SW
wave              (911)]    (916) L1    (906) L1
Trap                 \
(906)]                v
  |              [Blast Mine
  v                (912)]
[Claymore          needs: LM(904)L1
 Trap                + Sand(909)L1
 (907)]              + FT(911)L1
needs: SW(906)L1
  + BM(912)L1

Quest Skill:
[Phantasmic Arrow (917)] -- Job Lv 40 quest
```

### 8.3 Complete Prerequisite Table

| Skill (ID) | Prerequisites |
|-------------|---------------|
| **Archer** | |
| Owl's Eye (300) | None |
| Vulture's Eye (301) | Owl's Eye (300) Lv 3 |
| Improve Concentration (302) | Vulture's Eye (301) Lv 1 |
| Double Strafe (303) | None |
| Arrow Shower (304) | Double Strafe (303) Lv 5 |
| Arrow Crafting (305) | Quest (Job Lv 30) |
| Arrow Repel (306) | Quest (Job Lv 35) |
| **Hunter** | |
| Blitz Beat (900) | Falconry Mastery (916) Lv 1 |
| Steel Crow (901) | Blitz Beat (900) Lv **5** |
| Detect (902) | Improve Concentration (302) Lv 1, Falconry Mastery (916) Lv 1 |
| Ankle Snare (903) | Skid Trap (908) Lv 1 |
| Land Mine (904) | None |
| Remove Trap (905) | Land Mine (904) Lv 1 |
| Shockwave Trap (906) | Ankle Snare (903) Lv 1 |
| Claymore Trap (907) | Shockwave Trap (906) Lv 1, Blast Mine (912) Lv 1 |
| Skid Trap (908) | None |
| Sandman (909) | Flasher (910) Lv 1 |
| Flasher (910) | Skid Trap (908) Lv 1 |
| Freezing Trap (911) | Flasher (910) Lv 1 |
| Blast Mine (912) | Land Mine (904) Lv 1, Sandman (909) Lv 1, Freezing Trap (911) Lv 1 |
| Spring Trap (913) | Remove Trap (905) Lv 1, Falconry Mastery (916) Lv 1 |
| Talkie Box (914) | Shockwave Trap (906) Lv 1, Remove Trap (905) Lv 1 |
| Beast Bane (915) | None |
| Falconry Mastery (916) | Beast Bane (915) Lv 1 |
| Phantasmic Arrow (917) | Quest (Job Lv 40) |

### 8.4 Skill Point Investment Paths

**Maximum skill points:** 49 (job levels 1-49 for 2nd class)

**Trapper Build (Claymore Trap):**
```
Required chain: Skid 1 + Flasher 1 + Sandman 1 + Freezing 1 + Land Mine 1
              + Ankle Snare 1 + Shockwave 1 + Blast 1 + Claymore 5
= 13 skill points for Claymore Trap Lv 5
+ Remove Trap 1 = 14 for full trap utility
```

**Falcon Build (Blitz Beat + Steel Crow):**
```
Required chain: Beast Bane 1 + Falconry 1 + Blitz Beat 5 + Steel Crow 10
= 17 skill points for max falcon damage
```

**Hybrid (Falcon + Core Traps):**
```
Falcon core: BB1 + FM1 + BB5 + SC10 = 17
Ankle Snare: Skid1 + Ankle5 = 6
Land Mine 5: LM5 = 5
Remove Trap: LM1 (already) + RT1 = 1
Spring Trap: RT1 (already) + FM1 (already) = 0 extra
Detect: IC1 (Archer) + FM1 (already) + Det4 = 5
Total: 17 + 6 + 5 + 1 + 5 = 34 points, leaving 15 for other traps
```

---

## 9. Implementation Checklist

### 9.1 Core Systems

- [x] Ammunition/Arrow system (equip slot, consumption, element override)
- [x] Arrow Crafting recipe data file (`ro_arrow_crafting.js`)
- [x] Arrow Crafting handler (weight check, material selection, conversion)
- [ ] Trap Ground Entity System (`activeTraps` Map, TrapEntry struct)
- [ ] Trap placement validation (range, spacing, max trap limit)
- [ ] Trap trigger detection (enemy AI tick proximity check)
- [ ] Trap expiry/auto-detonate interval
- [ ] Trap socket events (placed, triggered, detonated, expired, removed)
- [ ] MISC damage type function (`calculateMiscDamage()`)
- [x] Falcon state tracking (`player.hasFalcon`)

### 9.2 Archer Skills

- [x] Owl's Eye (300) -- passive DEX bonus
- [x] Vulture's Eye (301) -- passive range + HIT bonus
- [x] Improve Concentration (302) -- self buff with stat source filtering
- [x] Double Strafe (303) -- bundled damage (single hit at total multiplier)
- [x] Arrow Shower (304) -- ground AoE with knockback
- [x] Arrow Crafting (305) -- item conversion
- [x] Arrow Repel (306) -- ranged knockback

### 9.3 Hunter Skills

- [x] Blitz Beat (900) -- MISC damage, manual cast
- [x] Steel Crow (901) -- passive falcon ATK
- [x] Detect (902) -- reveal hidden/cloaked
- [x] Ankle Snare (903) -- immobilize (NOT stun), boss 1/5 duration
- [x] Land Mine (904) -- Earth MISC + stun
- [x] Remove Trap (905) -- self trap removal + item return
- [x] Shockwave Trap (906) -- SP drain
- [x] Claymore Trap (907) -- Fire MISC, 5x5 AoE
- [x] Skid Trap (908) -- knockback only
- [x] Sandman (909) -- sleep, 5x5 AoE
- [x] Flasher (910) -- blind, single target
- [x] Freezing Trap (911) -- Water MISC + freeze
- [x] Blast Mine (912) -- Wind MISC + auto-detonate
- [x] Spring Trap (913) -- remote trap removal (falcon)
- [ ] Talkie Box (914) -- text message trap (needs client text input UI)
- [x] Beast Bane (915) -- passive race ATK
- [x] Falconry Mastery (916) -- falcon gate
- [x] Phantasmic Arrow (917) -- no-arrow attack with knockback
- [x] Auto-Blitz Beat -- LUK/3% proc, split damage, job level hits

### 9.4 Data Fixes Applied (from audit)

- [x] Blitz Beat formula: `(DEX/10 + INT/2 + SC*3 + 40) * 2` (was simplified incorrectly)
- [x] Auto-Blitz hit count: `floor((jobLv+9)/10)` (was off-by-one)
- [x] Ankle Snare: `ankle_snare` status type (was using `stun`)
- [x] Freezing Trap: verified damage type (MISC vs Weapon dispute noted)
- [x] Flasher trap cost: 1 (was incorrectly 2)
- [x] Freezing Trap cost: 1 (was incorrectly 2)
- [x] Steel Crow prereq: Blitz Beat Lv 5 (was Lv 1)
- [x] Detect: SP flat 8, prereqs added, range scales by level
- [x] Ankle Snare prereq: Skid Trap (was Land Mine)
- [x] Claymore prereq: Blast Mine (was Land Mine Lv 3)
- [x] Skid Trap prereqs: None (was Land Mine Lv 1)
- [x] Sandman prereq: Flasher (was Ankle Snare)
- [x] Blast Mine prereqs: LM1 + Sandman1 + FT1 (was LM5 only)
- [x] Spring Trap prereq: added Falconry Mastery
- [x] Talkie Box prereq: added Shockwave Trap
- [x] Falconry Mastery prereq: added Beast Bane
- [x] All trap cooldowns: 0 (were all incorrectly 1000ms)
- [x] Phantasmic Arrow: bow weapon check added

---

## 10. Gap Analysis

### 10.1 Implemented vs Canonical

| System | Implementation Status | Notes |
|--------|----------------------|-------|
| Owl's Eye | COMPLETE | Correct |
| Vulture's Eye | COMPLETE | Range scale may need UE unit review |
| Improve Concentration | COMPLETE | Stat source filtering implemented |
| Double Strafe | COMPLETE | Bundled damage, single hit |
| Arrow Shower | COMPLETE | 5x5 AoE, knockback implemented |
| Arrow Crafting | COMPLETE | 45 recipes in data file |
| Arrow Repel | COMPLETE | Knockback 6 cells |
| Blitz Beat | COMPLETE | Canonical rAthena formula |
| Auto-Blitz | COMPLETE | LUK/3%, split damage, job level hits |
| Steel Crow | COMPLETE | +6/level in Blitz formula |
| Detect | COMPLETE | 7x7 reveal |
| Ankle Snare | COMPLETE | ankle_snare status, boss 1/5, AGI reduction |
| Land Mine | COMPLETE | MISC damage, stun |
| Remove Trap | COMPLETE | Own traps only, item return |
| Shockwave Trap | COMPLETE | SP drain |
| Claymore Trap | COMPLETE | Highest damage, 5x5 |
| Skid Trap | COMPLETE | Knockback, deaggro |
| Sandman | COMPLETE | Sleep, 5x5 |
| Flasher | COMPLETE | Blind, single target |
| Freezing Trap | COMPLETE | Freeze + damage |
| Blast Mine | COMPLETE | Auto-detonate timer |
| Spring Trap | COMPLETE | Remote removal, falcon |
| Talkie Box | DEFERRED | Needs client text input UI |
| Beast Bane | COMPLETE | Race ATK bonus |
| Falconry Mastery | COMPLETE | Falcon gate |
| Phantasmic Arrow | COMPLETE | No arrow consumption |
| Arrow system | COMPLETE | Equip, consume, element override |
| Trap system | COMPLETE | Ground entities, lifecycle, triggers |
| MISC damage | COMPLETE | Separate from phys/mag pipelines |

### 10.2 Remaining Gaps / Deferred Features

| Feature | Priority | Reason Deferred |
|---------|----------|-----------------|
| Talkie Box text input | LOW | Needs client UI for text entry at cast time |
| Detect enemy trap reveal (PvP) | LOW | No PvP yet |
| Remove Trap cross-player (PvP) | LOW | No PvP yet |
| Falcon NPC rental cost (2,500z) | LOW | Currently granted on skill learn |
| Trap displacement by Arrow Shower | LOW | Edge case, rarely relevant |
| Arrow Crafting additional recipes | LOW | 45 recipes cover core; ~20 more from iRO Wiki could be added |
| Sniper transcendent skills | DEFERRED | Transcendent class not yet implemented |
| Beast Strafing / Soul Link | DEFERRED | Requires Soul Linker class |

### 10.3 Key Design Decisions

1. **Freezing Trap damage type:** Project uses MISC formula (pre-renewal community standard) rather than rAthena Weapon type. This matches the design principle that all trap damage ignores DEF/FLEE.

2. **Land Mine stun rate:** Uses `30 + 5 * SkillLv` (35-55%) per RateMyServer, not rAthena's `sc_start(10)` which could mean 10% flat. The higher rate is more commonly cited and makes the skill more impactful.

3. **Falcon rental:** Simplified to auto-grant on Falconry Mastery learn. The NPC rental interaction can be added later when NPC quest systems are more developed.

4. **UE unit conversion:** 1 RO cell = 50 UE units for range calculations. Knockback uses `knockbackTarget()` with its own cell size constant. Trap trigger radius = 50 UE (1 cell).

5. **Double Strafe hit display:** Implemented as single damage packet (1 hit) matching iRO Wiki "one single bundle" behavior, not as 2 separate visual hits.

---

## Web Research Sources

- [iRO Wiki Classic - Archer](https://irowiki.org/classic/Archer)
- [iRO Wiki Classic - Hunter](https://irowiki.org/classic/Hunter)
- [iRO Wiki - Double Strafe](https://irowiki.org/wiki/Double_Strafe)
- [iRO Wiki Classic - Double Strafe](https://irowiki.org/classic/Double_Strafe)
- [iRO Wiki - Arrow Shower](https://irowiki.org/wiki/Arrow_Shower)
- [iRO Wiki Classic - Arrow Shower](https://irowiki.org/classic/Arrow_Shower)
- [iRO Wiki Classic - Blitz Beat](https://irowiki.org/classic/Blitz_Beat)
- [iRO Wiki - Blitz Beat](https://irowiki.org/wiki/Blitz_Beat)
- [iRO Wiki Classic - Ankle Snare](https://irowiki.org/classic/Ankle_Snare)
- [iRO Wiki - Ankle Snare](https://irowiki.org/wiki/Ankle_Snare)
- [iRO Wiki - Land Mine](https://irowiki.org/wiki/Land_Mine)
- [iRO Wiki Classic - Land Mine](https://irowiki.org/classic/Land_Mine)
- [iRO Wiki - Blast Mine](https://irowiki.org/wiki/Blast_Mine)
- [iRO Wiki Classic - Blast Mine](https://irowiki.org/classic/Blast_Mine)
- [iRO Wiki - Claymore Trap](https://irowiki.org/wiki/Claymore_Trap)
- [iRO Wiki - Freezing Trap](https://irowiki.org/wiki/Freezing_Trap)
- [iRO Wiki Classic - Freezing Trap](https://irowiki.org/classic/Freezing_Trap)
- [iRO Wiki - Sandman](https://irowiki.org/wiki/Sandman)
- [iRO Wiki - Shockwave Trap](https://irowiki.org/wiki/Shockwave_Trap)
- [iRO Wiki - Remove Trap](https://irowiki.org/wiki/Remove_Trap)
- [iRO Wiki - Spring Trap](https://irowiki.org/wiki/Spring_Trap)
- [iRO Wiki - Phantasmic Arrow](https://irowiki.org/wiki/Phantasmic_Arrow)
- [iRO Wiki - Improve Concentration](https://irowiki.org/wiki/Improve_Concentration)
- [iRO Wiki Classic - Improve Concentration](https://irowiki.org/classic/Improve_Concentration)
- [iRO Wiki Classic - Arrow Crafting](https://irowiki.org/classic/Arrow_Crafting)
- [iRO Wiki - Arrow Crafting](https://irowiki.org/wiki/Arrow_Crafting)
- [iRO Wiki Classic - Boss Protocol](https://irowiki.org/classic/Boss_Protocol)
- [RateMyServer - Archer Skills](https://ratemyserver.net/index.php?page=skill_db&jid=3)
- [RateMyServer - Hunter Skills](https://ratemyserver.net/index.php?page=skill_db&jid=11)
- [RateMyServer - Double Strafe](https://ratemyserver.net/index.php?page=skill_db&skid=46)
- [RateMyServer - Blitz Beat](https://ratemyserver.net/index.php?page=skill_db&skid=129)
- [RateMyServer - Claymore Trap](https://ratemyserver.net/index.php?page=skill_db&skid=123)
- [RateMyServer - Blast Mine](https://ratemyserver.net/index.php?page=skill_db&skid=122)
- [RateMyServer - Ankle Snare](https://ratemyserver.net/index.php?page=skill_db&skid=117)
- [RateMyServer - Arrow Crafting Guide](https://ratemyserver.net/index.php?op=1&page=creation_db)
- [rAthena Pre-renewal DB - Blitz Beat](https://db.pservero.com/skill/HT_BLITZBEAT)
- [rAthena Pre-renewal DB - Ankle Snare](https://pre.pservero.com/skill/ht_anklesnare)
- [rAthena Pre-renewal DB - Blast Mine](https://pre.pservero.com/skill/HT_BLASTMINE)
- [rAthena Pre-renewal DB - Arrow Shower](https://db.pservero.com/skill/AC_SHOWER)
- [rAthena GitHub - skill.cpp](https://github.com/rathena/rathena/blob/master/src/map/skill.cpp)
- [rAthena GitHub - Blast Mine Behavior Fix](https://github.com/rathena/rathena/pull/3373)
- [rAthena GitHub - Ankle Snare Duration Issue](https://github.com/rathena/rathena/issues/8253)
- [rAthena GitHub - Auto-Blitz Damage Discussion](https://rathena.org/board/topic/141494-autoblitz-vs-blitz-active-damage/)
- [GameFAQs - Trap Hunter Guide](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/25790)
- [GameFAQs - Arrow Crafting FAQ](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/26381)
- [GameFAQs - Archer/Hunter FAQ](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/27633)
- [Project Alfheim Wiki - Blitz Beat](https://projectalfheim.net/wiki/index.php/Blitz_Beat)
- [Ragnarok Fandom Wiki - Blitz Beat](https://ragnarok.fandom.com/wiki/Blitz_Beat)
- [Ragnarok Fandom Wiki - Claymore Trap](https://ragnarok.fandom.com/wiki/Claymore_Trap)
- [Ragnarok Fandom Wiki - Freezing Trap](https://ragnarok.fandom.com/wiki/Freezing_Trap)
- [Origins RO Wiki - Blitz Beat](https://wiki.originsro.org/wiki/Blitz_Beat)
- [Ragnarok Zero Wiki - Blitz Beat](https://wiki.playragnarokzero.com/wiki/Blitz_Beat)
- [Neoseeker - Trap Hunter Guide](https://www.neoseeker.com/ragnarok-online/faqs/89955-trap-hunter.html)
- [ROGGH - Arrow Crafting](https://roggh.com/arrow-crafting/)
- [WarpPortal Forums - Auto-Blitz Hunter](https://forums.warpportal.com/index.php?/topic/15140-auto-blitz-hunter/)
