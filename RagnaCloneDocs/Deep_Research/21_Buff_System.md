# Buff System -- Deep Research (Pre-Renewal)

> Ragnarok Online Classic (pre-Renewal) complete buff system reference.
> Sources: rAthena pre-re source (status.cpp, status.hpp, skill.cpp, skill_db), iRO Wiki Classic, RateMyServer, divine-pride.net, Hercules emulator.
> Covers ALL supportive buffs, their durations, stacking rules, dispel interactions, and death persistence.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Complete Buff List](#2-complete-buff-list)
3. [Buff Categories](#3-buff-categories)
4. [Buff Stacking Rules](#4-buff-stacking-rules)
5. [Dispel Interaction](#5-dispel-interaction)
6. [Buffs That Survive Death](#6-buffs-that-survive-death)
7. [Buff Duration Mechanics](#7-buff-duration-mechanics)
8. [Buff Icons and Display](#8-buff-icons-and-display)
9. [Potion Buffs](#9-potion-buffs)
10. [Implementation Checklist](#10-implementation-checklist)
11. [Gap Analysis vs Sabri_MMO](#11-gap-analysis)

---

## 1. Overview

### How Buffs Work

In pre-renewal Ragnarok Online, buffs are temporary stat modifications applied to players or monsters through skills, items, or environmental effects. The game uses an internal system called **Status Changes** (SC_), where each buff/debuff has a unique SC_ identifier tracked in the server emulator.

The SC_ system works as follows:
1. A skill or item triggers `sc_start()` on the target
2. The server stores the status change with its type, duration, tick count, and associated values (val1-val4)
3. `status_calc_pc()` is called to recalculate all derived stats (ATK, DEF, ASPD, FLEE, etc.)
4. The client receives the icon update and displays it in the buff bar
5. On expiry (timer or hit count), `status_change_end()` is called, stats are recalculated, and the icon is removed

### SC_ System Architecture (rAthena)

```
status.hpp     -- SC_ enum definitions (SC_BLESSING = 30, SC_INCREASEAGI = 32, etc.)
status.cpp     -- status_change_start(), status_change_end(), status_calc_*() functions
status.txt     -- Status flag definitions (NoDispell, NoRemoveOnDead, NoClearance, etc.)
skill.cpp      -- Skill handlers that call sc_start() with appropriate values
skill_db.yml   -- Duration data per skill per level
```

Key architectural points:
- **val1-val4**: Each SC_ stores up to 4 integer values. Their meaning varies per status (e.g., for SC_BLESSING, val1 = skill level = stat bonus amount).
- **Status flags**: Each SC_ has flags controlling behavior: `NoDispell`, `NoRemoveOnDead`, `NoClearance`, `NoBanishingBuster`, `Debuff`, etc.
- **Status calc integration**: When any SC_ starts or ends, `status_calc_pc()` is triggered, iterating all active statuses to recompute every derived stat.

### Buff vs Status Effect Distinction

| Aspect | Buffs (Supportive SC_) | Status Effects (CC/DoT SC_) |
|--------|----------------------|---------------------------|
| Nature | Beneficial stat modifiers | Harmful crowd control / damage over time |
| Examples | Blessing, Increase AGI, Kyrie | Stun, Freeze, Poison, Bleeding |
| Resistance | None (always apply to valid targets) | Stat-based resistance formula |
| Stacking | Refresh (same name replaces) | No stacking (reject if already active) |
| Break on damage | No | Some (freeze, stone, sleep, confusion) |
| Cure/Remove | Dispel, specific counter-skills | Cure, Recovery, damage break |

---

## 2. Complete Buff List

Every supportive buff available in pre-renewal RO Classic, organized by source class. Each entry includes the SC_ identifier, source skill, duration formula, stat effects, and key mechanics.

---

### 2.1 Swordsman Buffs

#### SC_PROVOKE (Provoke) -- ID 6
- **Source**: Swordsman Lv1 skill "Provoke"
- **Duration**: 30 seconds (all levels)
- **Effects per level**:

| Lv | ATK Increase | DEF Reduction (soft VIT DEF) |
|----|-------------|------------------------------|
| 1  | +5%         | -10%                          |
| 2  | +8%         | -15%                          |
| 3  | +11%        | -20%                          |
| 4  | +14%        | -25%                          |
| 5  | +17%        | -30%                          |
| 6  | +20%        | -35%                          |
| 7  | +23%        | -40%                          |
| 8  | +26%        | -45%                          |
| 9  | +29%        | -50%                          |
| 10 | +32%        | -55%                          |

- **DEF target**: Reduces VIT-based soft DEF (def2), NOT equipment hard DEF (rAthena source confirmation)
- **Category**: Debuff (applied to enemies or self via Auto Berserk)
- **Cancels**: None
- **Cancelled by**: Provoke at any level (refresh), Decrease AGI (cancels Provoke)
- **Dispellable**: Yes

#### SC_ENDURE (Endure) -- ID 7
- **Source**: Swordsman Lv1 skill "Endure"
- **Duration**: 10 seconds (Swordsman) / 7 seconds (Knight/Crusader)
- **Effects**: Flinch immunity (prevents knockback/hitstun from physical attacks), +1 MDEF per skill level
- **Hit count**: Absorbs 7 hits (cancelled after 7th hit taken, OR on expiry, whichever comes first)
- **MDEF bonus**: +1 per skill level (Lv1=+1, Lv10=+10 MDEF)
- **Movement**: Allows movement while being hit (primary purpose)
- **Dispellable**: Yes
- **Survives death**: Yes (NoRemoveOnDead)

#### SC_AUTOGUARD (Auto Guard) -- ID 48 (Crusader)
- **Source**: Crusader skill "Auto Guard" (Guard)
- **Duration**: 300 seconds (5 minutes, all levels)
- **Effects**: Chance to block physical melee attacks with shield

| Lv | Block Chance |
|----|-------------|
| 1  | 5%          |
| 2  | 10%         |
| 3  | 14%         |
| 4  | 18%         |
| 5  | 21%         |
| 6  | 24%         |
| 7  | 26%         |
| 8  | 28%         |
| 9  | 29%         |
| 10 | 30%         |

- **Requires**: Shield equipped
- **Toggle**: Can be toggled on/off
- **Dispellable**: Yes

#### SC_REFLECTSHIELD (Reflect Shield) -- ID 67
- **Source**: Crusader skill "Shield Reflect"
- **Duration**: 300 seconds (5 minutes)
- **Effects**: Reflects (Lv * 3 + 7)% of physical melee damage back to attacker

| Lv | Reflect % |
|----|----------|
| 1  | 10%      |
| 2  | 13%      |
| 3  | 16%      |
| 4  | 19%      |
| 5  | 22%      |
| 6  | 25%      |
| 7  | 28%      |
| 8  | 31%      |
| 9  | 34%      |
| 10 | 37%      |

- **Reflected damage**: Based on physical melee damage received BEFORE reduction
- **Requires**: Shield equipped
- **Dispellable**: Yes

#### SC_DEFENDER (Defender / Defending Aura) -- ID 68
- **Source**: Crusader skill "Defending Aura"
- **Duration**: Toggle (infinite while active, costs SP to maintain)
- **Effects**: Reduces incoming ranged physical damage, reduces movement speed, reduces ASPD

| Lv | Ranged Reduction | Move Speed Penalty | ASPD Penalty |
|----|-----------------|-------------------|-------------|
| 1  | -20%            | -20%              | -25%        |
| 2  | -35%            | -17%              | -20%        |
| 3  | -50%            | -14%              | -15%        |
| 4  | -65%            | -11%              | -10%        |
| 5  | -80%            | -8%               | -5%         |

- **Toggle**: On/off, no SP drain while active (one-time SP cost on activation)
- **Dispellable**: Yes

#### SC_SHRINK (Shrink)
- **Source**: Crusader passive-toggle "Shrink"
- **Duration**: Toggle
- **Effects**: 50% chance to push back attackers by 2 cells when Auto Guard blocks
- **Requires**: Auto Guard active
- **Dispellable**: Yes
- **Survives death**: Yes (NoRemoveOnDead)

---

### 2.2 Acolyte / Priest Buffs

#### SC_BLESSING (Blessing) -- ID 30
- **Source**: Acolyte skill "Blessing"
- **Duration**: `(40 + 20 * Lv)` seconds

| Lv | STR/DEX/INT Bonus | HIT Bonus | Duration |
|----|-------------------|-----------|----------|
| 1  | +1                | +2        | 60s      |
| 2  | +2                | +4        | 80s      |
| 3  | +3                | +6        | 100s     |
| 4  | +4                | +8        | 120s     |
| 5  | +5                | +10       | 140s     |
| 6  | +6                | +12       | 160s     |
| 7  | +7                | +14       | 180s     |
| 8  | +8                | +16       | 200s     |
| 9  | +9                | +18       | 220s     |
| 10 | +10               | +20       | 240s     |

- **vs Undead/Demon**: Halves target's DEX and INT instead of buffing
- **Cures**: Curse and Stone (2nd stage Petrification) on application
- **Interaction**: Cancels Decrease AGI on target; Decrease AGI cancels Blessing
- **Dispellable**: Yes

#### SC_INCREASEAGI (Increase AGI) -- ID 32
- **Source**: Acolyte skill "Increase AGI"
- **Duration**: `(40 + 20 * Lv)` seconds (same formula as Blessing)

| Lv | AGI Bonus | Movement Speed | Duration |
|----|----------|----------------|----------|
| 1  | +3       | +25%           | 60s      |
| 2  | +4       | +25%           | 80s      |
| 3  | +5       | +25%           | 100s     |
| 4  | +6       | +25%           | 120s     |
| 5  | +7       | +25%           | 140s     |
| 6  | +8       | +25%           | 160s     |
| 7  | +9       | +25%           | 180s     |
| 8  | +10      | +25%           | 200s     |
| 9  | +11      | +25%           | 220s     |
| 10 | +12      | +25%           | 240s     |

- **Movement speed**: Fixed +25% at all levels (not scaling)
- **AGI bonus formula**: `(2 + Lv)` AGI
- **Interaction**: Cancels Decrease AGI on target; Decrease AGI cancels Increase AGI
- **Dispellable**: Yes

#### SC_DECREASEAGI (Decrease AGI) -- ID 33
- **Source**: Acolyte skill "Decrease AGI"
- **Duration**: `(40 + 20 * Lv)` seconds
- **Effects**: -(2 + Lv) AGI, -25% movement speed
- **Interaction**: Cancels Blessing, Increase AGI, Two-Hand Quicken, Spear Quicken, Adrenaline Rush, One-Hand Quicken on target
- **Category**: Debuff
- **Resistance**: VIT-based (pre-renewal)
- **Dispellable**: Yes

#### SC_ANGELUS (Angelus) -- ID 34
- **Source**: Acolyte skill "Angelus"
- **Duration**: `(30 * Lv)` seconds

| Lv | Soft DEF % Increase | Duration |
|----|-------------------|----------|
| 1  | +5%               | 30s      |
| 2  | +10%              | 60s      |
| 3  | +15%              | 90s      |
| 4  | +20%              | 120s     |
| 5  | +25%              | 150s     |
| 6  | +30%              | 180s     |
| 7  | +35%              | 210s     |
| 8  | +40%              | 240s     |
| 9  | +45%              | 270s     |
| 10 | +50%              | 300s     |

- **Target**: Self + all party members in screen
- **DEF type**: Increases VIT-based soft DEF by percentage
- **Dispellable**: Yes

#### SC_SIGNUMCRUCIS (Signum Crucis) -- ID 31
- **Source**: Acolyte skill "Signum Crucis"
- **Duration**: Permanent until death or zone change (no timer)
- **Effects**: Reduces Hard DEF of all Undead/Demon targets in screen by `(10 + 4 * Lv)`%
- **Category**: Debuff on targets
- **AoE**: All enemies in screen range
- **Dispellable**: No (applies to enemies)

#### SC_PNEUMA (Pneuma) -- ID 35 (Acolyte) / ground effect
- **Source**: Acolyte skill "Pneuma"
- **Duration**: 10 seconds (ground cell effect)
- **Effects**: All ranged physical attacks blocked for targets standing on the 3x3 cells
- **Type**: Ground effect, not a personal buff
- **Stacking**: Cannot overlap with Safety Wall or Land Protector
- **Dispellable**: N/A (ground effect)

#### SC_IMPOSITIO (Impositio Manus) -- ID 35
- **Source**: Priest skill "Impositio Manus"
- **Duration**: 60 seconds (all levels)

| Lv | Bonus ATK |
|----|----------|
| 1  | +5       |
| 2  | +10      |
| 3  | +15      |
| 4  | +20      |
| 5  | +25      |

- **ATK type**: Flat ATK bonus added post-DEF (like mastery bonuses)
- **Target**: Self + all party members in screen
- **Dispellable**: Yes

#### SC_SUFFRAGIUM (Suffragium) -- ID 36
- **Source**: Priest skill "Suffragium"
- **Duration**: 60 seconds (all levels, consumed on next spell cast)

| Lv | Cast Time Reduction |
|----|-------------------|
| 1  | -15%              |
| 2  | -30%              |
| 3  | -45%              |

- **Consumption**: Consumed when the buffed target starts casting any skill
- **Target**: Single ally (not self)
- **Stacking**: Does NOT stack with Poem of Bragi cast reduction (strongest wins)
- **Dispellable**: Yes

#### SC_ASPERSIO (Aspersio) -- ID 37
- **Source**: Priest skill "Aspersio"
- **Duration**: `(60 * Lv)` seconds (Lv1=60s, Lv5=300s)

| Lv | Duration |
|----|----------|
| 1  | 60s      |
| 2  | 120s     |
| 3  | 180s     |
| 4  | 240s     |
| 5  | 300s     |

- **Effects**: Changes weapon element to Holy
- **Interaction**: Overwrites and is overwritten by Enchant Poison, Endow skills, and elemental converters
- **Gemstone**: 1 Holy Water
- **Dispellable**: Yes

#### SC_BENEDICTIO (B.S. Sacramenti) -- ID 38
- **Source**: Priest skill "B.S. Sacramenti"
- **Duration**: `(40 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 40s      |
| 2  | 80s      |
| 3  | 120s     |
| 4  | 160s     |
| 5  | 200s     |

- **Effects**: Changes armor element to Holy Lv1
- **Target**: All players standing in the 3x3 area during cast
- **Dispellable**: Yes

#### SC_KYRIE (Kyrie Eleison) -- ID 39
- **Source**: Priest skill "Kyrie Eleison"
- **Duration**: 120 seconds (all levels)
- **Barrier HP**: `MaxHP * (10 + 2 * Lv)%`

| Lv | Barrier HP % of MaxHP | Max Hits Absorbed |
|----|----------------------|-------------------|
| 1  | 12%                  | 5                 |
| 2  | 14%                  | 6                 |
| 3  | 16%                  | 7                 |
| 4  | 18%                  | 8                 |
| 5  | 20%                  | 9                 |
| 6  | 22%                  | 10                |
| 7  | 24%                  | 11                |
| 8  | 26%                  | 12                |
| 9  | 28%                  | 13                |
| 10 | 30%                  | 14                |

- **Mechanic**: Absorbs damage hits. Breaks when total absorbed damage exceeds barrier HP OR max hit count is reached, OR on timer expiry.
- **Interaction**: Cancelled by any damage that breaks the barrier. Does NOT stack with Assumptio -- applying one removes the other.
- **Cannot coexist with**: Assumptio (SC_ASSUMPTIO)
- **Dispellable**: Yes

#### SC_MAGNIFICAT (Magnificat) -- ID 40
- **Source**: Priest skill "Magnificat"
- **Duration**: `(15 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 30s      |
| 2  | 45s      |
| 3  | 60s      |
| 4  | 75s      |
| 5  | 90s      |

- **Effects**: Doubles HP and SP natural regeneration rate (2x multiplier)
- **Target**: Self + all party members in screen
- **Dispellable**: Yes

#### SC_GLORIA (Gloria) -- ID 41
- **Source**: Priest skill "Gloria"
- **Duration**: `(5 * Lv)` seconds (Lv1=10s, Lv5=30s -- NOTE: some sources say 10 + 5*(Lv-1))

| Lv | LUK Bonus | Duration |
|----|----------|----------|
| 1  | +30      | 10s      |
| 2  | +30      | 15s      |
| 3  | +30      | 20s      |
| 4  | +30      | 25s      |
| 5  | +30      | 30s      |

- **Effects**: Flat +30 LUK at all levels (only duration scales)
- **Target**: Self + all party members in screen
- **Dispellable**: Yes

#### SC_AETERNA (Lex Aeterna) -- ID 42
- **Source**: Priest skill "Lex Aeterna"
- **Duration**: Until consumed (infinite timer, consumed on next damage instance)
- **Effects**: Next damage instance deals double damage (2x multiplier)
- **Consumption**: Removed after one hit of damage lands on the target
- **Target**: Single enemy
- **Category**: Debuff
- **Dispellable**: Yes

#### SC_SLOWPOISON (Slow Poison) -- ID 14
- **Source**: Priest skill "Slow Poison"
- **Duration**: `(10 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 10s      |
| 2  | 20s      |
| 3  | 30s      |
| 4  | 40s      |

- **Effects**: Suspends poison HP drain and DEF reduction; poison is NOT cured (resumes when Slow Poison expires unless Cure is used)
- **Dispellable**: Yes

---

### 2.3 Mage / Wizard Buffs

#### SC_SIGHT (Sight) -- ID 5
- **Source**: Mage skill "Sight"
- **Duration**: 10 seconds (all levels)
- **Effects**: Reveals hidden enemies (Hiding, Cloaking) in a 7x7 area around caster. Fire element damage dealt to revealed targets.
- **Dispellable**: Yes

#### SC_ENERGYCOAT (Energy Coat) -- ID 55
- **Source**: Mage quest skill "Energy Coat"
- **Duration**: 300 seconds (5 minutes, all levels) / some sources say infinite until dispelled
- **Effects**: Reduces incoming physical damage based on current SP percentage

| Current SP % | Damage Reduction | SP Consumed per Hit |
|-------------|-----------------|-------------------|
| 0-20%       | -6%             | -1%               |
| 21-40%      | -12%            | -2%               |
| 41-60%      | -18%            | -3%               |
| 61-80%      | -24%            | -4%               |
| 81-100%     | -30%            | -5%               |

- **SP cost**: Each hit received consumes SP proportional to damage reduction tier
- **Cancelled when**: SP reaches 0
- **Dispellable**: Yes

#### SC_SIGHTBLASTER (Sight Blaster) -- ID 406 (Wizard)
- **Source**: Wizard skill "Sight Blaster"
- **Duration**: 120 seconds
- **Effects**: Creates a fire-element ring around caster. When an enemy enters the 3x3 area, deals `(1 + Lv)` * 100% MATK fire damage and knocks them back 3 cells. Reactive trigger -- fires once then consumed.
- **Single use**: Consumed after triggering (one enemy triggers it)
- **Dispellable**: Yes

---

### 2.4 Thief / Assassin Buffs

#### SC_HIDING (Hiding) -- ID 6
- **Source**: Thief skill "Hiding"
- **Duration**: `(30 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 30s      |
| 5  | 150s     |
| 10 | 300s     |

- **Effects**: Character becomes invisible. Immune to most attacks. Prevents movement (except with Tunnel Drive passive).
- **SP drain**: 1 SP per 2 seconds while hidden
- **Break conditions**: Taking magic damage, being revealed by Ruwach/Sight/detection skills, casting skills, using items (some)
- **Dispellable**: Yes

#### SC_CLOAKING (Cloaking) -- ID 10
- **Source**: Assassin/Rogue skill "Cloaking"
- **Duration**: Until cancelled or SP depleted
- **Effects**: Character becomes invisible. Allows movement (unlike Hiding). Must be near walls (Lv1-2) or can move freely (Lv3+).
- **SP drain**: Varies by level (higher = less drain)
- **Movement speed**: Reduced while cloaked (less reduction at higher levels)
- **Break conditions**: Being revealed by detection skills, attacking, using skills
- **Dispellable**: Yes

#### SC_POISONREACT (Poison React) -- ID 17
- **Source**: Assassin skill "Poison React"
- **Duration**: `(20 + 4 * Lv)` seconds, OR consumed after triggering `(1 + Lv / 2)` times
- **Effects**: When hit by physical attack, has a chance to counter-attack with poison element damage and inflict Poison status
- **Trigger limit**: Consumed after the allotted number of counter-attacks
- **Dispellable**: Yes

#### SC_ENCPOISON (Enchant Poison) -- ID 16
- **Source**: Assassin skill "Enchant Poison"
- **Duration**: `(20 + 10 * Lv)` seconds (Lv1=30s, Lv10=120s)
- **Effects**: Adds Poison element to weapon attacks; each hit has a chance to inflict Poison status on target
- **Interaction**: Overwrites and is overwritten by Aspersio, elemental endows, converters
- **Dispellable**: Yes

---

### 2.5 Merchant / Blacksmith Buffs

#### SC_LOUD (Loud Exclamation / Crazy Uproar) -- ID 54
- **Source**: Merchant skill "Loud Exclamation" (Crazy Uproar)
- **Duration**: 300 seconds (5 minutes)
- **Effects**: +4 STR
- **Dispellable**: Yes

#### SC_ADRENALINE (Adrenaline Rush) -- ID 43
- **Source**: Blacksmith skill "Adrenaline Rush"
- **Duration**: `(30 * Lv)` seconds

| Lv | Duration | ASPD Bonus (Caster) | ASPD Bonus (Party) |
|----|----------|--------------------|--------------------|
| 1  | 30s      | +30%               | +25%               |
| 2  | 60s      | +30%               | +25%               |
| 3  | 90s      | +30%               | +25%               |
| 4  | 120s     | +30%               | +25%               |
| 5  | 150s     | +30%               | +25%               |

- **Weapon requirement**: Axe or Mace class weapons only
- **Party effect**: Party members with axes/maces get +25% ASPD (half effect)
- **Mutual exclusion**: ASPD Haste2 group (see Stacking Rules section)
- **Interaction**: Cancelled by Decrease AGI, Quagmire
- **Dispellable**: Yes

#### SC_WEAPONPERFECTION (Weapon Perfection) -- ID 44
- **Source**: Blacksmith skill "Weapon Perfection"
- **Duration**: `(10 * Lv)` seconds (Lv1=10s, Lv5=50s)
- **Effects**: Removes size penalty -- weapon deals 100% damage to all sizes (normally reduced for wrong-size weapons)
- **Target**: Self + party members (party gets same full effect)
- **Dispellable**: Yes

#### SC_OVERTHRUST (Power Thrust / Over Thrust) -- ID 45
- **Source**: Blacksmith skill "Power Thrust" (Over Thrust)
- **Duration**: `(20 * Lv)` seconds

| Lv | ATK Increase (Caster) | ATK Increase (Party) | Duration | Weapon Break Chance |
|----|----------------------|---------------------|----------|-------------------|
| 1  | +5%                  | +5%                 | 20s      | 0.1%              |
| 2  | +10%                 | +5%                 | 40s      | 0.2%              |
| 3  | +15%                 | +5%                 | 60s      | 0.3%              |
| 4  | +20%                 | +5%                 | 80s      | 0.4%              |
| 5  | +25%                 | +5%                 | 100s     | 0.5%              |

- **Party effect**: All party members get +5% ATK (flat, regardless of caster level)
- **Risk**: Each attack has a (0.1 * Lv)% chance to break the attacker's weapon
- **Dispellable**: Yes

#### SC_MAXIMIZEPOWER (Maximize Power) -- ID 46
- **Source**: Blacksmith skill "Maximize Power"
- **Duration**: Toggle (infinite while active)
- **Effects**: Weapon ATK always uses maximum damage roll (no random variance)
- **SP drain**: 1 SP per second while active (per attack in some implementations)
- **Cancelled when**: SP reaches 0, or manually toggled off
- **Dispellable**: Yes

---

### 2.6 Knight Buffs

#### SC_TWOHANDQUICKEN (Two-Hand Quicken) -- ID 18
- **Source**: Knight skill "Two-Hand Quicken"
- **Duration**: `(30 * Lv)` seconds

| Lv | Duration | ASPD Bonus | CRI Bonus | HIT Bonus |
|----|----------|-----------|----------|----------|
| 1  | 30s      | +30%      | +3       | +2       |
| 2  | 60s      | +30%      | +4       | +4       |
| 3  | 90s      | +30%      | +5       | +6       |
| 4  | 120s     | +30%      | +6       | +8       |
| 5  | 150s     | +30%      | +7       | +10      |
| 6  | 180s     | +30%      | +8       | +12      |
| 7  | 210s     | +30%      | +9       | +14      |
| 8  | 240s     | +30%      | +10      | +16      |
| 9  | 270s     | +30%      | +11      | +18      |
| 10 | 300s     | +30%      | +12      | +20      |

- **Weapon requirement**: Two-Handed Sword equipped
- **CRI formula**: `(2 + Lv)` critical rate bonus
- **HIT formula**: `(2 * Lv)` HIT bonus
- **ASPD**: Fixed +30% at all levels (only duration, CRI, HIT scale)
- **Mutual exclusion**: ASPD Haste2 group
- **Interaction**: Cancelled by Decrease AGI, Quagmire, weapon swap (to non-2H sword)
- **Dispellable**: Yes

#### SC_SPEARQUICKEN (Spear Quicken) -- ID 69 (Crusader)
- **Source**: Crusader/Knight skill "Spear Quicken"
- **Duration**: `(30 * Lv)` seconds

| Lv | Duration | ASPD Bonus | FLEE Bonus | CRI Bonus |
|----|----------|-----------|----------|----------|
| 1  | 30s      | +21%      | +2       | +3       |
| 2  | 60s      | +22%      | +4       | +3       |
| 3  | 90s      | +23%      | +6       | +3       |
| 4  | 120s     | +24%      | +8       | +3       |
| 5  | 150s     | +25%      | +10      | +3       |
| 6  | 180s     | +26%      | +12      | +3       |
| 7  | 210s     | +27%      | +14      | +3       |
| 8  | 240s     | +28%      | +16      | +3       |
| 9  | 270s     | +29%      | +18      | +3       |
| 10 | 300s     | +30%      | +20      | +3       |

- **Weapon requirement**: Spear equipped
- **ASPD formula**: `(20 + Lv)%` ASPD bonus (scales with level, unlike THQ)
- **Mutual exclusion**: ASPD Haste2 group
- **Interaction**: Cancelled by Decrease AGI, Quagmire, weapon swap
- **Dispellable**: Yes

---

### 2.7 Archer / Hunter Buffs

#### SC_CONCENTRATE (Improve Concentration) -- ID 19
- **Source**: Archer skill "Improve Concentration"
- **Duration**: `(40 + 20 * Lv)` seconds (same as Blessing/IncAGI formula)
- **Effects**: Increases AGI and DEX by `(1 + Lv)%` of base stat + reveals hidden enemies in screen

| Lv | AGI/DEX % Increase | Duration |
|----|-------------------|----------|
| 1  | +2%               | 60s      |
| 2  | +3%               | 80s      |
| 3  | +4%               | 100s     |
| 4  | +5%               | 120s     |
| 5  | +6%               | 140s     |
| 6  | +7%               | 160s     |
| 7  | +8%               | 180s     |
| 8  | +9%               | 200s     |
| 9  | +10%              | 220s     |
| 10 | +11%              | 240s     |

- **Reveal**: Reveals hidden enemies in screen on cast (like Sight, one-time check)
- **Dispellable**: Yes

---

### 2.8 Wizard Buffs (continued)

#### SC_LANDPROTECTOR (Land Protector)
- **Source**: Sage skill "Land Protector"
- **Duration**: Ground effect based on skill level
- **Effects**: Removes and prevents all ground-placed skills on the cells (Safety Wall, Pneuma, traps, etc.)
- **Type**: Ground effect

#### SC_VOLCANO / SC_DELUGE / SC_VIOLENTGALE (Sage Elemental Zones)
- **Source**: Sage skills "Volcano", "Deluge", "Violent Gale"
- **Duration**: Ground effect
- **Effects (while standing in zone)**:

| Skill | Effect on Fire/Water/Wind element weapons | Other effects |
|-------|------------------------------------------|---------------|
| Volcano | +[10, 20, 30, 40, 50] ATK (fire weapons) | +10-50% fire magic damage |
| Deluge | +[3, 6, 9, 12, 15]% MaxHP (water) | +10-50% water magic damage |
| Violent Gale | +[3, 6, 9, 12, 15] FLEE (wind) | +10-50% wind magic damage |

- **Element restriction**: ATK/HP/FLEE bonuses only apply to characters with matching element weapon or armor

---

### 2.9 Sage Buffs

#### SC_FLAMELAUNCHER / SC_FROSTWEAPON / SC_LIGHTNINGLOADER / SC_SEISMICWEAPON (Elemental Endows)
- **Source**: Sage skills "Endow Blaze/Tsunami/Tornado/Quake"
- **Duration**: `(20 * Lv)` seconds (Lv1=20s, Lv5=100s)
- **Effects**: Changes weapon element to Fire/Water/Wind/Earth respectively
- **Interaction**: Mutually exclusive with each other and with Aspersio, Enchant Poison, elemental converters. Applying one removes any other weapon element buff.
- **Dispellable**: Yes

#### SC_AUTOSPELL (Hindsight / Auto Spell) -- ID 20
- **Source**: Sage skill "Hindsight"
- **Duration**: `(60 * Lv)` seconds (Lv1=60s, Lv10=600s)
- **Effects**: Physical attacks have a chance to auto-cast a bolt spell (selected from learned spells)
- **Pre-renewal spell pool**: Fire Bolt, Cold Bolt, Lightning Bolt, Napalm Beat (Lv1 only), Soul Strike (at higher Hindsight levels)
- **Auto-cast chance**: Based on Hindsight level
- **Dispellable**: No (undispellable)

#### SC_MAGICROD (Magic Rod)
- **Source**: Sage skill "Magic Rod"
- **Duration**: Hit-based (absorbs 1 spell)
- **Effects**: Absorbs the next single-target magic spell cast at the user, converting it to SP recovery
- **SP recovery**: Equal to the SP cost of the absorbed spell
- **Consumption**: Consumed after absorbing one spell
- **Dispellable**: No

---

### 2.10 Monk Buffs

#### SC_EXPLOSIONSPIRITS (Fury / Critical Explosion) -- ID 47
- **Source**: Monk skill "Fury" (Critical Explosion)
- **Duration**: 180 seconds (3 minutes)
- **Effects**: +250% critical rate bonus (massively increases critical chance)
- **Side effect**: Blocks natural SP regeneration while active
- **Enables**: Allows use of Asura Strike, certain combo finishers
- **Dispellable**: Yes

#### SC_STEELBODY (Steel Body / Mental Strength) -- ID 56
- **Source**: Monk skill "Steel Body" (Mental Strength)
- **Duration**: 150 seconds (2.5 minutes)
- **Effects**:
  - Sets Hard DEF to 90 (90% physical damage reduction)
  - Sets Hard MDEF to 90 (90% magic damage reduction)
  - -25% ASPD
  - -25% Movement Speed
  - Blocks ALL active skill usage (can only auto-attack and walk)
- **Cannot be removed by**: Dispel (undispellable), Clearance
- **Cancelled by**: Timer expiry only
- **Dispellable**: No (undispellable)

---

### 2.11 Assassin Cross Buffs

#### SC_EDP (Enchant Deadly Poison) -- ID 73
- **Source**: Assassin Cross skill "Enchant Deadly Poison"
- **Duration**: `(40 + 4 * Lv)` seconds

| Lv | ATK Multiplier | Duration | Poison Chance |
|----|---------------|----------|--------------|
| 1  | +100%         | 44s      | Low          |
| 2  | +150%         | 48s      | Medium       |
| 3  | +200%         | 52s      | Medium-High  |
| 4  | +250%         | 56s      | High         |
| 5  | +300%         | 60s      | Very High    |

- **Pre-renewal formula**: `FinalDamage = Damage * (1 + EDP_Multiplier)` -- simple multiplier (NOT the renewal 4x/5x modifier)
- **Poison**: Attacks have additional chance to inflict deadly poison (stronger than regular poison)
- **Requires**: 1 Poison Bottle consumed per cast
- **Dispellable**: No (undispellable)

---

### 2.12 Rogue Buffs

#### SC_STRIPWEAPON / SC_STRIPSHIELD / SC_STRIPARMOR / SC_STRIPHELM (Divest / Full Strip)
- **Source**: Rogue skills "Divest Weapon/Shield/Armor/Helm"
- **Duration**: `(60 + 15 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 75s      |
| 2  | 90s      |
| 3  | 105s     |
| 4  | 120s     |
| 5  | 135s     |

- **Effects**: Unequips the target's weapon/shield/armor/helm and prevents re-equipping for the duration
- **Stat penalties**: Strip Weapon: -25% ATK; Strip Shield: -15% hard DEF; Strip Armor: -40% VIT; Strip Helm: -40% INT
- **Blocked by**: Chemical Protection (corresponding slot)
- **Category**: Debuff
- **Dispellable**: No (undispellable)

#### SC_CLOSECONFINE (Close Confine)
- **Source**: Rogue skill "Close Confine"
- **Duration**: 10 seconds (all levels)
- **Effects**: Locks both caster and target in place. Caster gains +10 FLEE. Neither can move.
- **Break conditions**: Knockback, teleport, Fly Wing, Butterfly Wing, Hiding, target or caster death
- **Dispellable**: No

---

### 2.13 Alchemist Buffs

#### SC_CP_WEAPON / SC_CP_SHIELD / SC_CP_ARMOR / SC_CP_HELM (Chemical Protection)
- **Source**: Alchemist skills "Chemical Protection Weapon/Shield/Armor/Helm" (Full Chemical Protection at Creator level)
- **Duration**: `(120 * Lv)` seconds

| Lv | Duration |
|----|----------|
| 1  | 120s     |
| 2  | 240s     |
| 3  | 360s     |
| 4  | 480s     |
| 5  | 600s     |

- **Effects**: Prevents the corresponding equipment slot from being broken (by Power Thrust weapon break, enemy skills) or stripped (by Rogue Divest skills)
- **Consumable**: 1 Glistening Coat per cast
- **Dispellable**: No (undispellable)

---

### 2.14 Bard / Dancer Buffs

All Bard/Dancer songs and dances create ground-based AoE effects that apply buffs to entities within range. The caster must stand still and continue performing (movement cancels). Buffs linger for 20 seconds after leaving the AoE.

#### Performance System Shared Mechanics
- **SP drain**: Continuous SP drain while performing (varies per skill)
- **Movement**: Movement speed reduced to `(25 + 2.5 * MusicLessons_Lv)%` while performing
- **Cancellation**: Moving, weapon swap, Dispel, silence, heavy damage (>25% MaxHP in one hit), Adaptation to Circumstances
- **Mutual exclusion**: Only one song/dance active at a time per performer

#### SC_WHISTLE (A Whistle)
- **Source**: Bard skill "A Whistle"
- **Duration**: While in AoE + 20s linger
- **Effects**: +FLEE and +Perfect Dodge to allies in range
- **FLEE formula**: `(BaseLv + DEX) / 10 + Lv * 2`
- **PD formula**: `(BaseLv + LUK) / 10`

#### SC_ASSNCROS (Assassin Cross of Sunset)
- **Source**: Bard skill "Assassin Cross of Sunset"
- **Duration**: While in AoE + 20s linger
- **Effects**: +ASPD to allies in range (Haste2 group member)
- **ASPD formula**: Based on Bard's AGI and DEX
- **Mutual exclusion**: ASPD Haste2 group (strongest wins vs THQ/AR/SQ)

#### SC_POEMBRAGI (Poem of Bragi)
- **Source**: Bard skill "Poem of Bragi"
- **Duration**: While in AoE + 20s linger
- **Effects**: Reduces cast time and after-cast delay for allies in range
- **Cast reduction formula**: Based on Bard's DEX and skill level
- **ACD reduction formula**: Based on Bard's INT and skill level

#### SC_APPLEIDUN (Apple of Idun)
- **Source**: Bard skill "Apple of Idun"
- **Duration**: While in AoE + 20s linger
- **Effects**: +MaxHP% and HP regeneration to allies in range
- **MaxHP formula**: Based on Bard's VIT and skill level

#### SC_HUMMING (Humming)
- **Source**: Dancer skill "Humming"
- **Duration**: While in AoE + 20s linger
- **Effects**: +HIT to allies in range
- **HIT formula**: `(2 * Lv) + floor(BaseLv + DEX) / 10`

#### SC_DONTFORGETME (Please Don't Forget Me)
- **Source**: Dancer skill "Please Don't Forget Me"
- **Duration**: While in AoE + 20s linger
- **Effects**: -ASPD and -Movement Speed to enemies in range
- **Category**: Debuff

#### SC_FORTUNEKISS (Fortune's Kiss)
- **Source**: Dancer skill "Fortune's Kiss"
- **Duration**: While in AoE + 20s linger
- **Effects**: +Critical Rate to allies in range
- **CRI formula**: `Lv + floor(LUK + BaseLv) / 10`

#### SC_SERVICEFORYOU (Service for You)
- **Source**: Dancer skill "Service for You"
- **Duration**: While in AoE + 20s linger
- **Effects**: +MaxSP% and -SP cost to allies in range
- **MaxSP formula**: Based on Dancer's INT and skill level
- **SP cost reduction**: Based on Dancer's INT and skill level

---

### 2.15 Ensemble Buffs (Bard + Dancer cooperative)

Ensembles require both a Bard and Dancer performing together. They create stationary ground effects at the midpoint of the two performers. Both performers must remain within range.

#### SC_DRUMBATTLEFIELD (A Drum on the Battlefield)
- **Effects**: +ATK and +DEF to allies in range
- **ATK formula**: `(Bard_ATK + Dancer_ATK) / 5 * Lv`
- **DEF formula**: `(Bard_DEF + Dancer_DEF) / 5 * Lv`

#### SC_RINGNIBELUNGEN (The Ring of Nibelungen)
- **Effects**: +ATK for wielders of Lv4 weapons in range
- **ATK formula**: `(Music_Lessons_Bard + Music_Lessons_Dancer) * 5 * Lv`

#### SC_ROKAIHOU (Into the Abyss)
- **Effects**: Gemstone consumption removed for all skills cast within the AoE

#### SC_INTOABYSS (Mr. Kim A Rich Man)
- **Effects**: +EXP gain % for all party members in range

#### SC_SIEGFRIED (Invulnerable Siegfried)
- **Effects**: Elemental resistance (non-Neutral) and status effect resistance to allies

| Lv | Element Resist | Status Resist |
|----|---------------|--------------|
| 1  | 60%           | 10%          |
| 2  | 65%           | 15%          |
| 3  | 70%           | 20%          |
| 4  | 75%           | 25%          |
| 5  | 80%           | 50%          |

#### SC_MOONLITWATER (Moonlit Water Mill)
- **Effects**: Prevents all attacks (friendly and hostile) within the AoE. Nobody can attack while standing in it.

#### SC_LULLABY (Lullaby)
- **Effects**: Chance to inflict Sleep on enemies in the AoE every tick. Allies (same party as performers) are immune.

---

### 2.16 Crusader Buffs (continued)

#### SC_DEVOTION (Devotion) -- ID 70
- **Source**: Crusader skill "Devotion"
- **Duration**: `(15 * Lv)` seconds (Lv1=30s, Lv5=90s -- some sources: +15s per level)

| Lv | Duration | Max Targets |
|----|----------|-------------|
| 1  | 30s      | 1           |
| 2  | 45s      | 2           |
| 3  | 60s      | 3           |
| 4  | 75s      | 4           |
| 5  | 90s      | 5           |

- **Effects**: All physical and magical damage taken by the devotion target is redirected to the Crusader instead
- **Range**: 6 cells (target must stay within this range or link breaks)
- **Requirements**: Target must be in same party, target's base level must be within 10 levels of Crusader
- **Break conditions**: Target moves out of range, CC on Crusader (stun/freeze/stone/sleep), Crusader dies
- **Dispellable**: No (undispellable)

#### SC_PROVIDENCE (Providence)
- **Source**: Crusader skill "Providence"
- **Duration**: `(15 * Lv)` seconds (Lv1=30s, Lv5=90s -- same as Devotion)
- **Effects**: +Demon race resistance +Holy element resistance

| Lv | Demon Resist | Holy Resist |
|----|-------------|-------------|
| 1  | +5%         | +5%         |
| 2  | +10%        | +10%        |
| 3  | +15%        | +15%        |
| 4  | +20%        | +20%        |
| 5  | +25%        | +25%        |

- **Dispellable**: Yes

---

### 2.17 Misc Buffs

#### SC_TRICKDEAD (Play Dead / Trick Dead) -- ID 52
- **Source**: Novice skill "Play Dead" (also learned via other classes)
- **Duration**: Until cancelled (toggle)
- **Effects**: Character appears dead to all monsters. All monsters deaggro (including bosses). Cannot move, attack, or use skills while active.
- **Dispellable**: No (undispellable)

#### SC_RUWACH (Ruwach) -- ID 53
- **Source**: Acolyte skill "Ruwach"
- **Duration**: 10 seconds
- **Effects**: Reveals hidden enemies in 5x5 area around caster. Deals holy element damage to revealed targets.
- **Dispellable**: Yes

#### SC_AUTOBERSERK (Auto Berserk) -- ID 51
- **Source**: Swordsman skill "Auto Berserk" (passive toggle)
- **Duration**: Toggle (activates automatically when HP drops below 25%)
- **Effects**: When HP < 25%, applies self-Provoke Lv10 (+32% ATK, -55% soft DEF)
- **Deactivates**: When HP rises to >= 25% (effects removed)
- **Dispellable**: No (undispellable)
- **Survives death**: Yes (NoRemoveOnDead) -- the toggle stays, so it reactivates on revival at low HP

#### SC_WEDDING (Wedding Dress)
- **Source**: Wedding event item
- **Duration**: Until login/zone change or manually removed
- **Effects**: Changes appearance to wedding outfit. Reduces movement speed.
- **Dispellable**: No

#### SC_RIDING (Peco Riding)
- **Source**: Knight/Crusader skill "Riding"
- **Duration**: Toggle (until dismounted)
- **Effects**: +36% movement speed (1.36x multiplier). Modifies sprite. Changes spear damage vs Medium targets to 100%.
- **Dispellable**: No

#### SC_CART (Pushcart)
- **Source**: Merchant skill "Pushcart"
- **Duration**: Until removed (toggle)
- **Effects**: Enables cart use. Reduces movement speed based on cart weight.
- **Dispellable**: No

---

## 3. Buff Categories

### 3.1 Stat Buffs

Buffs that directly modify character stats (STR, AGI, VIT, INT, DEX, LUK):

| Buff | Stats Modified | Flat/% | Max Bonus |
|------|---------------|--------|----------|
| Blessing | STR, DEX, INT, HIT | Flat | +10 each, +20 HIT |
| Increase AGI | AGI | Flat | +12 AGI |
| Gloria | LUK | Flat | +30 LUK |
| Loud Exclamation | STR | Flat | +4 STR |
| Improve Concentration | AGI, DEX | % of base | +11% each |
| Stat Foods | One stat each | Flat | +1 to +10 |
| Provoke | ATK%, DEF% | % modifier | +32% ATK, -55% DEF |
| Quagmire | AGI, DEX | Flat reduction | -5 to -50 each |

### 3.2 ASPD Buffs

Buffs that increase Attack Speed:

| Buff | ASPD Bonus | Weapon Requirement | Class Restriction |
|------|----------|-------------------|------------------|
| Two-Hand Quicken | +30% | 2H Sword | Knight/LK |
| Spear Quicken | +21-30% | Spear | Crusader/Paladin |
| Adrenaline Rush | +30% (caster), +25% (party) | Axe/Mace | Blacksmith/WS |
| Assassin Cross of Sunset | Variable | Any | Bard song AoE |
| Concentration Potion | Tier 0 ASPD | None | All |
| Awakening Potion | Tier 1 ASPD | None | All (1st class+) |
| Berserk Potion | Tier 2 ASPD | None | All (2nd class+) |

### 3.3 Defensive Buffs

Buffs that reduce incoming damage or provide protection:

| Buff | Defense Type | Mechanic |
|------|-------------|----------|
| Kyrie Eleison | Hit-based barrier | Absorbs hits up to MaxHP% threshold |
| Safety Wall | Ground cell protection | Blocks melee attacks on cell (hit count) |
| Pneuma | Ground cell protection | Blocks ranged attacks on 3x3 cells |
| Auto Guard | Chance-based block | 5-30% to block melee attacks |
| Reflect Shield | Damage reflection | 10-37% of damage reflected to attacker |
| Defender | Ranged reduction | 20-80% ranged physical damage reduction |
| Energy Coat | SP-based reduction | 6-30% based on SP% (consumes SP per hit) |
| Steel Body | Hard DEF/MDEF override | Sets DEF and MDEF to 90 |
| Angelus | Soft DEF % increase | +5-50% VIT-based defense |
| Endure | Flinch immunity | 7-hit flinch immunity + MDEF bonus |
| Providence | Race/Element resist | +5-25% vs Demon and Holy |
| Chemical Protection (x4) | Equipment protection | Prevents break/strip |
| Devotion | Damage redirect | All damage redirected to Crusader |
| Invulnerable Siegfried | Element/Status resist | 60-80% element resist, 10-50% status resist |

### 3.4 Offensive Buffs

Buffs that increase damage output:

| Buff | ATK Bonus | Mechanic |
|------|----------|---------|
| Impositio Manus | +5 to +25 flat ATK | Post-DEF flat bonus |
| Power Thrust | +5-25% ATK (caster), +5% (party) | Multiplicative ATK% |
| Maximize Power | Max weapon variance | Always max damage roll |
| Enchant Deadly Poison | +100-300% ATK | Massive pre-renewal multiplier |
| Provoke (self) | +5-32% ATK | Via Auto Berserk at low HP |
| Lex Aeterna | 2x next hit | Consumed on first damage |
| Drum on Battlefield | Flat ATK bonus | Ensemble buff |
| Ring of Nibelungen | Flat ATK (Lv4 weapon) | Ensemble buff, weapon-locked |

### 3.5 Movement Buffs

Buffs that affect movement speed:

| Buff | Speed Modifier | Notes |
|------|---------------|-------|
| Increase AGI | +25% | Fixed at all levels |
| Peco Riding | +36% | Knight/Crusader mount |
| Cart Boost | +100% (Sprint) | Mastersmith skill (Trans) |
| Decrease AGI (debuff) | -25% | Cancels movement buffs |
| Quagmire (debuff) | -50% at max | 3-layer speed reduction |
| Defender | -8 to -20% | Trade-off for ranged reduction |
| Steel Body | -25% | Trade-off for DEF |
| Pushcart | Varies by weight | Merchant cart penalty |

### 3.6 Resistance Buffs

Buffs that provide resistance to elements, races, or status effects:

| Buff | Resistance Type | Amount |
|------|----------------|--------|
| Providence | Demon race + Holy element | +5-25% each |
| Invulnerable Siegfried | All elements (non-Neutral) + Status | 60-80% element, 10-50% status |
| B.S. Sacramenti | Armor Holy element | Changes armor to Holy Lv1 |
| Chemical Protection (x4) | Equipment break/strip | Full immunity per slot |
| Endure | Flinch resistance | 7-hit immunity |

### 3.7 Weapon Element Buffs

Buffs that change weapon attack element (mutually exclusive group):

| Buff | Element | Source |
|------|---------|--------|
| Aspersio | Holy | Priest skill |
| Enchant Poison | Poison | Assassin skill |
| Endow Blaze | Fire | Sage skill |
| Endow Tsunami | Water | Sage skill |
| Endow Tornado | Wind | Sage skill |
| Endow Quake | Earth | Sage skill |
| Fire Converter | Fire | Consumable item |
| Water Converter | Water | Consumable item |
| Wind Converter | Wind | Consumable item |
| Earth Converter | Earth | Consumable item |
| Cursed Water | Dark | Consumable item |

**Rule**: Only ONE weapon element buff can be active at a time. Applying any new one removes the old one.
**Priority**: Skill endow > Item converter > Weapon innate element

---

## 4. Buff Stacking Rules

### 4.1 General Stacking Principle

In pre-renewal RO, the default rule is **refresh**: re-casting the same buff replaces the old instance (resetting duration and updating values to the new level). Two instances of the same buff never coexist.

Different buffs generally DO coexist unless they belong to a mutual exclusion group.

### 4.2 Mutual Exclusion Groups

#### ASPD Haste2 Group (Critical -- only strongest wins)

These four ASPD buffs are mutually exclusive. Only the one providing the highest ASPD bonus takes effect:

| Buff | ASPD Bonus | Priority Notes |
|------|----------|----------------|
| Two-Hand Quicken | +30% | Knight (2H Sword) |
| Spear Quicken | +21-30% | Crusader (Spear) |
| Adrenaline Rush | +30% caster / +25% party | Blacksmith (Axe/Mace) |
| Assassin Cross of Sunset | Variable | Bard song AoE |

**Rule**: If multiple are active (e.g., Adrenaline Rush from party + own Spear Quicken), only the one with the highest ASPD% applies. They do NOT stack additively.

**Cancelled by**: All four are cancelled by Decrease AGI and Quagmire.

**ASPD Potions**: ASPD potions (Concentration/Awakening/Berserk) are a SEPARATE system from the Haste2 group. Potions use a fixed ASPD calculation method, while Haste2 uses a percentage modifier. They coexist -- a character can have both Berserk Potion AND Two-Hand Quicken active.

#### Weapon Element Group (Mutually Exclusive)

Only one weapon element override can be active at a time:
- Aspersio, Enchant Poison, Endow Blaze/Tsunami/Tornado/Quake, Fire/Water/Wind/Earth Converter, Cursed Water

Applying any one removes all others.

#### Kyrie Eleison vs Assumptio

These two cannot coexist. Casting one removes the other.

#### Blessing vs Decrease AGI

These cancel each other. Casting Blessing on a Decrease AGI target removes Decrease AGI, and vice versa.

#### Increase AGI vs Decrease AGI

These cancel each other. Casting Increase AGI on a Decrease AGI target removes Decrease AGI, and vice versa.

### 4.3 Stacking -- What DOES Coexist

These buffs all stack (coexist) without issues:

- Blessing + Increase AGI + Gloria + Angelus + Magnificat (standard priest party buff set)
- Blessing + Impositio Manus + Aspersio + Kyrie Eleison
- Two-Hand Quicken + Berserk Potion (Haste2 + potion = different systems)
- Stat Foods + Blessing (flat stat bonuses from different sources stack)
- Power Thrust + Maximize Power + Weapon Perfection (Blacksmith triple-buff)
- Auto Guard + Reflect Shield + Defender (Crusader triple-buff, though all have trade-offs)
- Energy Coat + Kyrie Eleison (different damage reduction mechanics)
- Provoke (on target) + Lex Aeterna (different debuff types)

### 4.4 Song/Dance Stacking

- Only ONE song/dance can be active per performer at a time. Starting a new one cancels the old.
- A character can benefit from multiple different songs/dances simultaneously (e.g., A Whistle from one Bard + Poem of Bragi from another Bard)
- **Overlap rule**: If two songs/dances of the SAME TYPE overlap on the same cell, they produce Dissonance (damage ticks) instead of the intended effect
- **Ensemble vs Solo**: A Bard performing a solo song can also be in the AoE of another Bard's song. The second song's buff applies normally.

---

## 5. Dispel Interaction

### 5.1 How Dispel Works

Sage skill "Dispel" (Dispell, ID 1413) removes all dispellable buffs from the target. It has a (50 + 10 * Lv)% success rate and consumes 1 Yellow Gemstone.

### 5.2 Dispellable Buffs (Removed by Dispel)

The following buffs ARE removed by Dispel:

```
SC_PROVOKE          (Provoke)
SC_ENDURE           (Endure)
SC_TWOHANDQUICKEN   (Two-Hand Quicken)
SC_CONCENTRATE      (Improve Concentration)
SC_HIDING           (Hiding)
SC_CLOAKING         (Cloaking)
SC_ENCPOISON        (Enchant Poison)
SC_POISONREACT      (Poison React)
SC_QUAGMIRE         (Quagmire)
SC_ANGELUS          (Angelus)
SC_BLESSING         (Blessing)
SC_SIGNUMCRUCIS     (Signum Crucis)
SC_INCREASEAGI      (Increase AGI)
SC_DECREASEAGI      (Decrease AGI)
SC_SLOWPOISON       (Slow Poison)
SC_IMPOSITIO        (Impositio Manus)
SC_SUFFRAGIUM       (Suffragium)
SC_ASPERSIO         (Aspersio)
SC_BENEDICTIO       (B.S. Sacramenti)
SC_KYRIE            (Kyrie Eleison)
SC_MAGNIFICAT       (Magnificat)
SC_GLORIA           (Gloria)
SC_AETERNA          (Lex Aeterna)
SC_ADRENALINE       (Adrenaline Rush)
SC_WEAPONPERFECTION (Weapon Perfection)
SC_OVERTHRUST       (Power Thrust)
SC_MAXIMIZEPOWER    (Maximize Power)
SC_LOUD             (Loud Exclamation)
SC_ENERGYCOAT       (Energy Coat)
SC_SIGHT            (Sight)
SC_RUWACH           (Ruwach)
SC_AUTOGUARD        (Auto Guard)
SC_REFLECTSHIELD    (Reflect Shield)
SC_DEFENDER         (Defender)
SC_SPEARQUICKEN     (Spear Quicken)
SC_PROVIDENCE       (Providence)
SC_EXPLOSIONSPIRITS (Fury)
SC_FLAMELAUNCHER    (Endow Blaze)
SC_FROSTWEAPON      (Endow Tsunami)
SC_LIGHTNINGLOADER  (Endow Tornado)
SC_SEISMICWEAPON    (Endow Quake)
SC_WHISTLE          (A Whistle - song buff on recipient)
SC_ASSNCROS         (Assassin Cross of Sunset - song buff)
SC_POEMBRAGI        (Poem of Bragi - song buff)
SC_APPLEIDUN        (Apple of Idun - song buff)
SC_HUMMING          (Humming - dance buff)
SC_DONTFORGETME     (Please Don't Forget Me)
SC_FORTUNEKISS      (Fortune's Kiss - dance buff)
SC_SERVICEFORYOU    (Service for You - dance buff)
SC_ASPDPOTION0/1/2  (ASPD Potions - Concentration/Awakening/Berserk)
SC_ATKPOTION        (ATK Potion)
SC_MATKPOTION       (MATK Potion)
SC_SIGHTBLASTER     (Sight Blaster)
SC_VOLCANO/DELUGE/VIOLENTGALE (zone buffs on recipient)
```

### 5.3 Undispellable Buffs (NOT removed by Dispel)

The following buffs are IMMUNE to Dispel (have NoDispell flag in rAthena):

```
SC_AUTOSPELL        (Hindsight / Auto Spell)
SC_TRICKDEAD        (Play Dead)
SC_AUTOBERSERK      (Auto Berserk)
SC_DEVOTION         (Devotion)
SC_STEELBODY        (Steel Body / Mental Strength)
SC_EDP              (Enchant Deadly Poison)
SC_CP_WEAPON        (Chemical Protection Weapon)
SC_CP_SHIELD        (Chemical Protection Shield)
SC_CP_ARMOR         (Chemical Protection Armor)
SC_CP_HELM          (Chemical Protection Helm)
SC_STRIPWEAPON      (Divest Weapon)
SC_STRIPSHIELD      (Divest Shield)
SC_STRIPARMOR       (Divest Armor)
SC_STRIPHELM        (Divest Helm)
SC_CLOSECONFINE     (Close Confine)
SC_COMBO            (Combo state - Monk)
SC_WEDDING          (Wedding Dress)
SC_RIDING           (Peco Riding)
SC_CART             (Pushcart)
SC_BERSERK          (LK Berserk / Frenzy - Transcendent)
SC_ASSUMPTIO        (Assumptio - Trans only)
SC_MAGICROD         (Magic Rod)
SC_BLADESTOP        (Blade Stop)
```

### 5.4 Special Dispel Immunities

- **Golden Thief Bug Card**: Players with GTB card shield are completely immune to Dispel
- **Soul Linker**: Soul Linkers are immune to Dispel
- **Rogue Spirit**: Stalkers/Shadow Chasers linked with Rogue Spirit are immune to Dispel

---

## 6. Buffs That Survive Death

### 6.1 rAthena NoRemoveOnDead Flag

In pre-renewal RO, most buffs are cleared when the player dies. The following have the `NoRemoveOnDead` flag and persist through death:

```
SC_ENDURE           (Endure)
SC_AUTOBERSERK      (Auto Berserk toggle)
SC_SHRINK           (Shrink toggle)
SC_RIDING           (Peco Riding)
SC_CART             (Pushcart)
SC_WEDDING          (Wedding)
```

### 6.2 Buffs Cleared on Death

Everything not in the above list is removed on death, including:
- All stat buffs (Blessing, Increase AGI, Gloria, Angelus, etc.)
- All ASPD buffs (THQ, AR, SQ, ACoS, potions)
- All element endows (Aspersio, Enchant Poison, Sage endows)
- All defensive buffs (Kyrie, Auto Guard, Defender, Reflect Shield, Energy Coat, Steel Body)
- All offensive buffs (Impositio, Power Thrust, Maximize Power, EDP)
- All songs/dances (performance state cancelled)
- All debuffs (Provoke, Quagmire, Strip, Lex Aeterna)
- All status effects (Stun, Freeze, Poison, etc.)
- All potion buffs (ASPD potions, stat foods, ATK/MATK potions)

### 6.3 Special Death Interactions

- **Redemptio**: Priest skill that sacrifices the caster to resurrect all dead party members. Caster dies and loses 1% base EXP.
- **Kaizel**: Transcendent Priest buff that auto-revives the target once on death (consumed on death)
- **Token of Siegfried**: Item that prevents EXP loss on death (consumed on death)

---

## 7. Buff Duration Mechanics

### 7.1 Time-Based Buffs

Most buffs have a fixed duration in seconds, often scaling with skill level. Common formulas:

| Formula Pattern | Skills Using It |
|----------------|----------------|
| `(40 + 20 * Lv)` seconds | Blessing, Increase AGI, Decrease AGI, Improve Concentration |
| `(30 * Lv)` seconds | Two-Hand Quicken, Spear Quicken, Adrenaline Rush, Angelus |
| `(60 * Lv)` seconds | Aspersio, Hindsight |
| `(15 * Lv)` seconds | Magnificat, Devotion, Providence |
| `(20 * Lv)` seconds | Power Thrust, Elemental Endows |
| `(10 * Lv)` seconds | Weapon Perfection, Slow Poison |
| `(5 * Lv)` seconds | Gloria (5 + 5*Lv in some sources) |
| `(120 * Lv)` seconds | Chemical Protection (x4) |
| Fixed 30s | Provoke |
| Fixed 60s | Impositio Manus, Suffragium |
| Fixed 120s | Kyrie Eleison |
| Fixed 300s | Auto Guard, Reflect Shield, Loud Exclamation |
| Fixed 180s | Fury (Critical Explosion) |
| Fixed 150s | Steel Body |

### 7.2 Hit-Based Buffs

Some buffs are consumed after a certain number of hits:

| Buff | Hit Count | Additional Timer |
|------|----------|-----------------|
| Endure | 7 hits | 10s (also expires on timer) |
| Kyrie Eleison | 5-14 hits (by level) | 120s (also expires on timer) |
| Lex Aeterna | 1 hit (consumed on first damage) | Infinite (no timer) |
| Suffragium | 1 cast (consumed on next spell) | 60s (also expires on timer) |
| Poison React | 1-5 triggers (by level) | 20-40s (also expires on timer) |
| Sight Blaster | 1 trigger | 120s |

### 7.3 Toggle Buffs (Infinite Duration)

Some buffs persist until manually cancelled or a break condition is met:

| Buff | Cancel Condition |
|------|-----------------|
| Maximize Power | Manual toggle off, SP reaches 0 |
| Defender | Manual toggle off |
| Auto Guard | Manual toggle off |
| Hiding | Manual toggle, reveal, magic damage, skill use |
| Cloaking | Manual toggle, reveal, attack, SP depleted |
| Play Dead | Manual toggle |
| Auto Berserk | HP rises above 25% (auto-deactivates) |
| Riding | /mount command |
| Steel Body | Timer (150s) only -- cannot be manually cancelled |

### 7.4 Potion Buff Durations

| Potion | Duration |
|--------|----------|
| Concentration Potion | 1800s (30 minutes) |
| Awakening Potion | 1800s (30 minutes) |
| Berserk Potion | 1800s (30 minutes) |
| Stat Foods (all) | 1200s (20 minutes) |
| ATK Potion | 60s (1 minute) |
| MATK Potion | 60s (1 minute) |
| Speed Potion | 5s (brief sprint) |
| Authoritative Badge | 180s (3 minutes) |

---

## 8. Buff Icons and Display

### 8.1 Icon Display Location

Status icons appear on the **right-hand side** of the player's screen in a vertical column. Icons are small (16x16 or 24x24 pixel sprites) stacked top-to-bottom.

### 8.2 Icon Information Toggle

The `/stateinfo` command toggles display of status icon descriptions. When enabled, hovering over or clicking an icon shows:
- Buff name
- Remaining duration (countdown timer)
- Brief description of effects

### 8.3 Icon Categories

Icons use different border colors to distinguish buff types:
- **Green border**: Beneficial buffs (Blessing, Increase AGI, etc.)
- **Red border**: Debuffs/harmful effects (Provoke, Stun, Poison, etc.)
- **Yellow border**: Neutral/special effects (Wedding, Riding, etc.)

### 8.4 Maximum Buff Display

The client has a practical limit on displayed icons based on screen vertical resolution. With many buffs active, icons can extend below the visible screen area. This was a known UI limitation in classic RO.

### 8.5 Implementation Notes for Sabri_MMO

The Sabri_MMO `BuffBarSubsystem` uses a horizontal buff bar at the top of the screen. Each buff displays:
- 3-character abbreviation (from `BUFF_TYPES[].abbrev`)
- Category coloring (buff = green, debuff = red)
- Remaining time countdown
- DisplayName on hover/tooltip

---

## 9. Potion Buffs

### 9.1 ASPD Potions

ASPD potions provide a fixed attack speed increase through a separate calculation path from skill-based ASPD buffs. They use the `SC_ASPDPOTION0/1/2/3` status changes.

| Potion | SC_ Code | Duration | ASPD Effect | Class Restriction |
|--------|----------|----------|------------|-------------------|
| Concentration Potion | SC_ASPDPOTION0 | 1800s | Tier 0 (weakest) | All classes |
| Awakening Potion | SC_ASPDPOTION1 | 1800s | Tier 1 (medium) | 1st class and above |
| Berserk Potion | SC_ASPDPOTION2 | 1800s | Tier 2 (strongest) | 2nd class and above |
| Poison Bottle | SC_ASPDPOTION3 | 60s | Tier 3 (special) | Assassin Cross (EDP use) |

**Stacking within potions**: Only the strongest ASPD potion takes effect. Using a stronger potion replaces the weaker one. Using a weaker potion while a stronger one is active does nothing.

**Stacking with skills**: ASPD potions DO coexist with skill-based ASPD buffs (THQ, AR, SQ, ACoS). The potion and skill bonuses are computed separately and both contribute to final ASPD.

### 9.2 Stat Foods

Stat foods provide temporary flat stat bonuses. They share the same SC_ per stat type (e.g., all STR foods use SC_STRFOOD), so only the strongest STR food is active at a time.

| SC_ Code | Stat | Items (examples) | Bonus Range | Duration |
|----------|------|-------------------|------------|----------|
| SC_STRFOOD | STR | Fried Grasshopper Legs (+1) to Lutie Lady's Pancake (+5) | +1 to +10 | 1200s |
| SC_AGIFOOD | AGI | Nut Eaten By Squirrel (+1) to Soup With Meat On The Bone (+5) | +1 to +10 | 1200s |
| SC_VITFOOD | VIT | Spicy Fried Bao (+1) to Steamed Bat Wing In Pumpkin (+5) | +1 to +10 | 1200s |
| SC_INTFOOD | INT | Grape Juice Herbal Tea (+1) to Chocolate Mousse Cake (+5) | +1 to +10 | 1200s |
| SC_DEXFOOD | DEX | Fried Monkey Tail (+1) to Broiled Down Trout (+5) | +1 to +10 | 1200s |
| SC_LUKFOOD | LUK | Fried Sweet Potato (+1) to Fried Scorpion Tail (+5) | +1 to +10 | 1200s |
| SC_HITFOOD | HIT | Various | +1 to +10 | 1200s |
| SC_FLEEFOOD | FLEE | Various | +1 to +10 | 1200s |

**Stacking with skill buffs**: Stat foods DO stack with skill buffs (e.g., SC_STRFOOD +5 STR + SC_BLESSING +10 STR = +15 STR total). They use different SC_ codes and are tracked independently.

**Stacking within stat foods**: Only one food per stat type can be active. Using a new STR food replaces the old STR food (refresh rule).

### 9.3 Combat Potions

| SC_ Code | Effect | Duration | Source Items |
|----------|--------|----------|-------------|
| SC_ATKPOTION | +ATK (flat bonus) | 60s | Box of Resentment (+20), Distilled Fighting Spirit (+30), Durian (+10) |
| SC_MATKPOTION | +MATK (flat bonus) | 60s | Box of Drowsiness (+20), Herb of Incantation (+30), Durian (+10) |

### 9.4 Speed Items

| SC_ Code | Effect | Duration | Source Items |
|----------|--------|----------|-------------|
| SC_SPEEDUP0 | +25% movement speed | 180s / 20s | Authoritative Badge (180s), Box of Thunder (20s) |
| SC_SPEEDUP1 | +50% movement speed | 5s | Speed Potion (5s) |
| SC_SLOWDOWN | -100% movement speed | 5s | Slow Potion (5s, debuff) |

### 9.5 Elemental Converter Items

| Item | Element | Duration | SC_ Code |
|------|---------|----------|----------|
| Flame Elemental Converter | Fire | 1800s | SC_FIREWEAPON |
| Frost Elemental Converter | Water | 1800s | SC_WATERWEAPON |
| Lightning Elemental Converter | Wind | 1800s | SC_WINDWEAPON |
| Seismic Elemental Converter | Earth | 1800s | SC_EARTHWEAPON |
| Cursed Water | Dark | 1800s | SC_SHADOWWEAPON |

**Interaction**: These use the same mutually-exclusive weapon element slot as Aspersio, Enchant Poison, and Sage endows. Only one can be active.

---

## 10. Implementation Checklist

### 10.1 Core Buff System Requirements

- [x] `applyBuff()` with refresh/stack/reject stack rules
- [x] `removeBuff()` by name
- [x] `expireBuffs()` called from 1s tick
- [x] `hasBuff()` check
- [x] `getBuffModifiers()` aggregating all active buff stats
- [x] `getActiveBuffList()` for client serialization
- [x] `getCombinedModifiers()` merging status effects + buffs
- [x] Buff bar display (BuffBarSubsystem)
- [x] `skill:buff_applied` / `skill:buff_removed` events

### 10.2 Buff Types -- Implementation Status

#### Swordsman
- [x] Provoke (SC_PROVOKE)
- [x] Endure (SC_ENDURE)
- [x] Auto Berserk (SC_AUTOBERSERK)
- [x] Auto Guard (SC_AUTOGUARD)
- [x] Reflect Shield (SC_REFLECTSHIELD)
- [x] Defender (SC_DEFENDER)
- [x] Shrink (SC_SHRINK)

#### Acolyte / Priest
- [x] Blessing (SC_BLESSING)
- [x] Increase AGI (SC_INCREASEAGI)
- [x] Decrease AGI (SC_DECREASEAGI)
- [x] Angelus (SC_ANGELUS)
- [x] Signum Crucis (SC_SIGNUMCRUCIS)
- [x] Impositio Manus (SC_IMPOSITIO)
- [x] Suffragium (SC_SUFFRAGIUM)
- [x] Aspersio (SC_ASPERSIO)
- [x] B.S. Sacramenti (SC_BENEDICTIO)
- [x] Kyrie Eleison (SC_KYRIE)
- [x] Magnificat (SC_MAGNIFICAT)
- [x] Gloria (SC_GLORIA)
- [x] Lex Aeterna (SC_AETERNA)
- [x] Slow Poison (SC_SLOWPOISON)
- [x] Pneuma (ground effect)

#### Mage / Wizard
- [x] Sight (SC_SIGHT)
- [x] Energy Coat (SC_ENERGYCOAT)
- [x] Sight Blaster (SC_SIGHTBLASTER)

#### Thief / Assassin
- [x] Hiding (SC_HIDING)
- [x] Cloaking (SC_CLOAKING)
- [x] Enchant Poison (SC_ENCPOISON)
- [x] Poison React (SC_POISONREACT)
- [x] Enchant Deadly Poison (SC_EDP) -- may need verification

#### Merchant / Blacksmith
- [x] Loud Exclamation (SC_LOUD)
- [x] Adrenaline Rush (SC_ADRENALINE)
- [x] Weapon Perfection (SC_WEAPONPERFECTION)
- [x] Power Thrust (SC_OVERTHRUST)
- [x] Maximize Power (SC_MAXIMIZEPOWER)

#### Knight / Crusader
- [x] Two-Hand Quicken (SC_TWOHANDQUICKEN)
- [x] Spear Quicken (SC_SPEARQUICKEN)
- [x] Devotion (SC_DEVOTION)
- [x] Providence (SC_PROVIDENCE)

#### Sage
- [x] Elemental Endows (x4) (SC_FLAMELAUNCHER etc.)
- [x] Hindsight (SC_AUTOSPELL)
- [x] Magic Rod (SC_MAGICROD)
- [x] Volcano/Deluge/Violent Gale zones

#### Monk
- [x] Fury / Critical Explosion (SC_EXPLOSIONSPIRITS)
- [x] Steel Body (SC_STEELBODY)

#### Bard / Dancer
- [x] A Whistle (SC_WHISTLE)
- [x] Assassin Cross of Sunset (SC_ASSNCROS)
- [x] Poem of Bragi (SC_POEMBRAGI)
- [x] Apple of Idun (SC_APPLEIDUN)
- [x] Humming (SC_HUMMING)
- [x] Please Don't Forget Me (SC_DONTFORGETME)
- [x] Fortune's Kiss (SC_FORTUNEKISS)
- [x] Service for You (SC_SERVICEFORYOU)

#### Ensemble
- [x] Drum on the Battlefield (SC_DRUMBATTLEFIELD)
- [x] Ring of Nibelungen (SC_RINGNIBELUNGEN)
- [x] Into the Abyss (SC_ROKAIHOU)
- [x] Mr. Kim A Rich Man
- [x] Invulnerable Siegfried (SC_SIEGFRIED)

#### Alchemist
- [x] Chemical Protection (x4) (SC_CP_WEAPON etc.)

#### Rogue
- [x] Divest / Strip (x4) (SC_STRIPWEAPON etc.)
- [x] Close Confine (SC_CLOSECONFINE)

#### Consumable Buffs
- [x] ASPD Potions (SC_ASPDPOTION0/1/2)
- [x] Stat Foods (SC_STRFOOD through SC_LUKFOOD)
- [x] Elemental Converters (item endows)
- [x] ATK/MATK Potions
- [x] Speed items

#### Misc
- [x] Play Dead (SC_TRICKDEAD)
- [x] Ruwach (SC_RUWACH)
- [x] Magnum Break Fire (temporary fire endow)
- [ ] Wedding (SC_WEDDING) -- not gameplay-critical
- [x] Riding (SC_RIDING)
- [x] Cart (SC_CART) -- via cart system

### 10.3 Stacking Rules Implementation

- [x] ASPD Haste2 mutual exclusion group (THQ/SQ/AR/ACoS -- strongest wins)
- [x] Weapon element mutual exclusion (only one active)
- [x] Decrease AGI cancels: Blessing, Increase AGI, THQ, SQ, AR
- [x] Dispel removes all dispellable buffs
- [x] UNDISPELLABLE set correctly defined
- [x] BUFFS_SURVIVE_DEATH set defined
- [x] `clearBuffsOnDeath()` called on all player death paths

### 10.4 Missing/Not Yet Needed

- [ ] SC_ASSUMPTIO (Assumptio) -- Transcendent class only
- [ ] SC_BERSERK (LK Frenzy) -- Transcendent class only
- [ ] SC_KAAHI (Kaahi) -- Soul Linker only
- [ ] SC_KAITE (Kaite) -- Soul Linker only
- [ ] SC_KAUPE (Kaupe) -- Soul Linker only
- [ ] SC_KAIZEL (Kaizel) -- Transcendent class only
- [ ] Gospel / Battle Chant -- Transcendent class only
- [ ] Soul Links (various) -- Soul Linker / Super Novice only
- [ ] SC_HALLUCINATION (Hallucination) -- from Confusion status
- [ ] SC_WEIGHT50 / SC_WEIGHT90 -- Weight penalty statuses (implemented differently)

---

## 11. Gap Analysis

### 11.1 Current Sabri_MMO vs RO Classic Accuracy

Comparing the Sabri_MMO `ro_buff_system.js` implementation against this reference:

#### Correctly Implemented
- Provoke: Soft DEF reduction (rAthena-verified), ATK% increase
- Blessing: STR/DEX/INT bonus per level, correct duration formula
- Increase AGI: AGI bonus + movement speed, correct duration
- Haste2 group: THQ/SQ/AR/ACoS mutual exclusion with strongest-wins
- ASPD potion separation: Potions use different calculation path from Haste2
- Weapon element mutual exclusion: Only one endow active
- Dispel: UNDISPELLABLE set includes all correct entries
- Death: BUFFS_SURVIVE_DEATH set covers key buffs
- Stat foods: Separate SC_ per stat, refresh within same type
- Chemical Protection: Undispellable, correct duration formula
- Ensembles: Stationary AoE, correct stat formulas
- Invulnerable Siegfried: Per-element resist + status resist (dual values)

#### Potential Gaps to Verify
1. **Endure hit count**: Verify 7-hit limit is tracked and decremented
2. **Kyrie Eleison hit count**: Verify max hits tracked alongside barrier HP
3. **Suffragium consumption**: Verify consumed on next spell cast
4. **Poison React trigger count**: Verify consumed after allotted triggers
5. **Energy Coat SP tiers**: Verify 5-tier SP% lookup table
6. **Gloria LUK**: Confirm +30 flat at all levels (not scaling)
7. **Blessing vs Undead/Demon**: Verify DEX/INT halving for Undead/Demon targets
8. **Decrease AGI strip list**: Should cancel THQ, SQ, AR, ACoS, Blessing, Increase AGI
9. **Song linger duration**: 20-second buff linger after leaving AoE
10. **Provoke DEF type**: Must reduce soft (VIT) DEF, NOT hard (equipment) DEF -- confirmed in codebase

#### Not Yet Relevant (Transcendent/Future)
- Assumptio, Kaizel, Gospel, LK Berserk, Soul Links -- these are Transcendent class or Soul Linker features not yet in scope

---

## Sources

- [iRO Wiki - Status Effects](https://irowiki.org/wiki/Status_Effects)
- [iRO Wiki Classic - Status Icons](https://irowiki.org/classic/Status_Icons)
- [iRO Wiki - Buffs](https://irowiki.org/wiki/Buffs)
- [iRO Wiki - Blessing](https://irowiki.org/wiki/Blessing)
- [iRO Wiki Classic - Blessing](https://irowiki.org/classic/Blessing)
- [iRO Wiki - Increase AGI](https://irowiki.org/wiki/Increase_AGI)
- [iRO Wiki Classic - Increase AGI](https://irowiki.org/classic/Increase_AGI)
- [iRO Wiki - Twohand Quicken](https://irowiki.org/wiki/Twohand_Quicken)
- [iRO Wiki Classic - Twohand Quicken](https://irowiki.org/classic/Twohand_Quicken)
- [iRO Wiki - Adrenaline Rush](https://irowiki.org/wiki/Adrenaline_Rush)
- [iRO Wiki - ASPD](https://irowiki.org/wiki/ASPD)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [iRO Wiki - Dispell](https://irowiki.org/wiki/Dispell)
- [iRO Wiki Classic - Dispell](https://irowiki.org/classic/Dispell)
- [iRO Wiki - Impositio Manus](https://irowiki.org/wiki/Impositio_Manus)
- [iRO Wiki - Kyrie Eleison](https://irowiki.org/wiki/Kyrie_Eleison)
- [iRO Wiki Classic - Kyrie Eleison](https://irowiki.org/classic/Kyrie_Eleison)
- [iRO Wiki - Safety Wall](https://irowiki.org/wiki/Safety_Wall)
- [iRO Wiki - Enchant Deadly Poison](https://irowiki.org/wiki/Enchant_Deadly_Poison)
- [iRO Wiki Classic - Enchant Deadly Poison](https://irowiki.org/classic/Enchant_Deadly_Poison)
- [iRO Wiki - Guard (Auto Guard)](https://irowiki.org/wiki/Guard)
- [iRO Wiki - Defending Aura](https://irowiki.org/wiki/Defending_Aura)
- [iRO Wiki - Spear Quicken](https://irowiki.org/wiki/Spear_Quicken)
- [iRO Wiki - Gloria](https://irowiki.org/wiki/Gloria)
- [iRO Wiki - Full Chemical Protection](https://irowiki.org/wiki/Full_Chemical_Protection)
- [iRO Wiki Classic - Full Chemical Protection](https://irowiki.org/classic/Full_Chemical_Protection)
- [rAthena GitHub - status.hpp](https://github.com/rathena/rathena/blob/master/src/map/status.hpp)
- [rAthena GitHub - status.cpp](https://github.com/rathena/rathena/blob/master/src/map/status.cpp)
- [rAthena GitHub - status_change.txt](https://github.com/rathena/rathena/blob/master/doc/status_change.txt)
- [rAthena GitHub - status.txt](https://github.com/rathena/rathena/blob/master/doc/status.txt)
- [rAthena Wiki - Status_List](https://github.com/rathena/rathena/wiki/Status_List)
- [rAthena Wiki - Status_Change_End](https://github.com/rathena/rathena/wiki/Status_Change_End)
- [rAthena Board - SC Undispellable List](https://rathena.org/board/topic/96279-sc-undispellable-list)
- [rAthena Board - Buffs after death](https://rathena.org/board/topic/114556-buffs-after-death-active/)
- [rAthena Board - Status effects after death](https://rathena.org/board/topic/139676-status-effects-after-death/)
- [rAthena Skill DB - Gloria](https://db.pservero.com/skill/PR_GLORIA)
- [rAthena Skill DB - Aspersio](https://db.pservero.com/skill/PR_ASPERSIO/)
- [rAthena Skill DB - Magnificat](https://db.pservero.com/skill/PR_MAGNIFICAT)
- [rAthena Skill DB - Increase AGI](https://db.pservero.com/skill/AL_INCAGI)
- [rAthena pre-re skill_db.txt](https://github.com/flaviojs/rathena-commits/blob/master/db/pre-re/skill_db.txt)
- [RateMyServer - Provoke](https://ratemyserver.net/index.php?page=skill_db&skid=6)
- [RateMyServer - Endure](https://ratemyserver.net/index.php?page=skill_db&skid=8)
- [RateMyServer - Angelus](https://ratemyserver.net/index.php?page=skill_db&skid=33)
- [RateMyServer - Guide: Official Status Resistance Formulas (Pre-Renewal)](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [RateMyServer - ASPD Potion Stacking](https://forum.ratemyserver.net/general-discussion/stackable-aspd-pots/)
- [RateMyServer - Blacksmith Skill DB](https://ratemyserver.net/index.php?page=skill_db&jid=18)
- [Ragnarok Wiki Fandom - Enchant Deadly Poison](https://ragnarok.fandom.com/wiki/Enchant_Deadly_Poison)
- [Ragnarok Wiki Fandom - Two-Hand Quicken](https://ragnarok.fandom.com/wiki/Two-Hand_Quicken)
- [WarpPortal Forums - Buffs upon death](https://forums.warpportal.com/index.php?/topic/111593-buffs-upon-death/)
- [EDP Damage Bonus Issue #8187](https://github.com/rathena/rathena/issues/8187)
- [RagnaPlace - Blessing](https://ragnaplace.com/en/wiki/irowiki/Blessing)
- [RagnaPlace - Increase AGI](https://ragnaplace.com/en/wiki/irowiki/Increase%20AGI)
- [RagnaPlace - Adrenaline Rush](https://ragnaplace.com/en/wiki/irowiki/Adrenaline_Rush)
- [Ragnarok Wiki Fandom - Devotion](https://ragnarok.fandom.com/wiki/Devotion)
