# Status Effects & Debuffs -- Deep Research (Pre-Renewal)

> **Sources**: iRO Wiki Classic, rAthena pre-renewal source (`status.cpp`, `status.hpp`, `status_change.txt`), RateMyServer Official Pre-Renewal Status Resistance Formulas guide (Playtester 2013), Hercules pre-re, Ragnarok Wiki (Fandom), divine-pride.net, idRO Klasik Wiki, Project Alfheim Wiki, TalonRO Wiki.
> **Scope**: Every negative status effect available in pre-Renewal Ragnarok Online (Episodes 1-14.1, before June 2010 Renewal patch). Excludes Renewal-only effects (Burning, Crystallization, Deep Sleep, Fear, Mandragora Howling, etc.).

---

## Table of Contents

1. [Overview](#1-overview)
2. [Status Effect Application Model](#2-status-effect-application-model)
3. [Universal Resistance Formula](#3-universal-resistance-formula)
4. [Major Status Ailments (10 Core)](#4-major-status-ailments-10-core)
   - [Stun](#41-stun)
   - [Freeze](#42-freeze)
   - [Stone Curse (Petrification)](#43-stone-curse-petrification)
   - [Sleep](#44-sleep)
   - [Poison](#45-poison)
   - [Deadly Poison (SC_DPOISON)](#46-deadly-poison-sc_dpoison)
   - [Blind (Darkness)](#47-blind-darkness)
   - [Silence](#48-silence)
   - [Confusion (Chaos)](#49-confusion-chaos)
   - [Bleeding](#410-bleeding)
   - [Curse](#411-curse)
5. [Secondary Debuffs (Skill-Based)](#5-secondary-debuffs-skill-based)
   - [Provoke](#51-provoke)
   - [Lex Aeterna](#52-lex-aeterna)
   - [Lex Divina (Silence via Skill)](#53-lex-divina)
   - [Signum Crucis](#54-signum-crucis)
   - [Decrease AGI](#55-decrease-agi)
   - [Quagmire](#56-quagmire)
   - [Hallucination](#57-hallucination)
   - [Coma](#58-coma)
   - [Strip/Divest Effects](#59-strip-divest-effects)
   - [Ankle Snare (Root)](#510-ankle-snare-root)
   - [Close Confine](#511-close-confine)
   - [Spider Web](#512-spider-web)
   - [Enchant Deadly Poison (EDP Debuff)](#513-enchant-deadly-poison-edp-debuff)
   - [Safety Wall / Pneuma Expiry](#514-safety-wall--pneuma-expiry)
   - [Ensemble Aftermath](#515-ensemble-aftermath)
6. [Status Effect Resistance Formulas (Detailed)](#6-status-effect-resistance-formulas-detailed)
   - [VIT-Based Resistance](#61-vit-based-resistance)
   - [INT-Based Resistance](#62-int-based-resistance)
   - [LUK-Based Resistance](#63-luk-based-resistance)
   - [MDEF-Based Resistance](#64-mdef-based-resistance)
   - [Composite Stat Resistance](#65-composite-stat-resistance)
7. [Boss Protocol Status Immunity](#7-boss-protocol-status-immunity)
8. [Frozen = Water Lv1, Stone = Earth Lv1](#8-frozen--water-lv1-stone--earth-lv1)
9. [Status Immunity Items & Cards](#9-status-immunity-items--cards)
10. [Status Effects from Weapons & Arrows](#10-status-effects-from-weapons--arrows)
11. [Cure Methods Reference Table](#11-cure-methods-reference-table)
12. [Dispel Interaction](#12-dispel-interaction)
13. [Implementation Checklist](#13-implementation-checklist)
14. [Gap Analysis vs Sabri_MMO](#14-gap-analysis-vs-sabri_mmo)

---

## 1. Overview

Ragnarok Online pre-Renewal has a rich status effect system where negative conditions ("status ailments") can be applied to both players and monsters by skills, items, weapon procs, and card effects. The system is divided into two categories:

**Core Status Ailments (OPT1/OPT2 flags)**: 11 hardcoded negative status effects with standardized resistance formulas. These are the "classic" CC (crowd control) and DoT (damage over time) effects. They are represented in the rAthena source as SC_STONE (0), SC_FREEZE (1), SC_STUN (2), SC_SLEEP (3), SC_POISON (4), SC_CURSE (5), SC_SILENCE (6), SC_CONFUSION (7), SC_BLIND (8), SC_BLEEDING (9), and SC_DPOISON (10).

**Skill-Based Debuffs (SC_ status changes)**: Dozens of negative skill effects implemented as status changes (buffs with debuff category). These include Provoke, Lex Aeterna, Quagmire, Decrease AGI, Strip effects, and many more. They do not use the standard resistance formula -- each has its own application logic.

### Key Principles

1. **No stacking**: A target cannot have two instances of the same status ailment simultaneously. Re-application is rejected while one is active.
2. **Resistance is stat-based**: Each ailment has a primary resist stat (VIT, INT, LUK, or MDEF). Higher stat = lower chance of affliction and shorter duration.
3. **LUK universal resistance**: LUK provides minor resistance to ALL status ailments. 300 LUK grants total immunity to all core status ailments except Curse (which uses LUK as primary resist).
4. **Level difference matters**: Higher-level attackers have better infliction rates against lower-level targets.
5. **Boss protocol**: Monsters with boss/MVP flag are immune to most core status ailments.
6. **Element change**: Freeze changes target to Water Lv1; Stone changes target to Earth Lv1. This is a critical combat mechanic.
7. **Damage break**: Some statuses (Freeze, Stone, Sleep, Confusion) are broken when the target takes damage.
8. **Item cure**: Most statuses can be cured by specific consumable items (Green Herb, Panacea, Royal Jelly, Holy Water).

---

## 2. Status Effect Application Model

### Application Flow (rAthena pre-renewal)

```
1. Skill/Card/Weapon triggers status application
2. Check target boss immunity (modeFlags.statusImmune) -> reject if boss
3. Check target already has this status -> reject (no stacking)
4. Check target equipment immunity (Marc Card, Orc Hero Card, etc.)
5. Calculate resistance chance:
   FinalChance = BaseChance - (BaseChance * ResistStat / 100) + srcLv - tarLv - tarLUK
   Clamp to [5, 95]
6. Roll random(100) < FinalChance -> if fail, reject
7. Calculate duration:
   Duration = BaseDuration - (BaseDuration * ResistStat / 200) - 10 * tarLUK
   Minimum 1000ms
8. Apply status: create entry in target.activeStatusEffects
9. Broadcast to zone: status:applied event
```

### OPT1 vs OPT2 in rAthena

In the rAthena source, status ailments are split into two client-side flag groups:

**OPT1 (mutually exclusive hard CC)**:
- Stone (1), Freeze (2), Stun (3), Sleep (4), Petrifying/Stone Curse Phase 1 (6)
- Only ONE OPT1 status can be active at a time. Applying a new OPT1 status replaces the old one.

**OPT2 (stackable soft effects)**:
- Poison (0x0001), Curse (0x0002), Silence (0x0004), Confusion/Signum Crucis (0x0008), Blind (0x0010), Bleeding (0x0040), Deadly Poison (0x0080)
- Multiple OPT2 effects can coexist on the same target.

### Damage Break Mechanic

When a target under a breakable status takes any damage (physical, magical, or misc), the status is immediately removed. The breakable statuses are:
- **Freeze**: Broken by ANY damage
- **Stone** (Phase 2 / fully petrified): Broken by ANY damage
- **Sleep**: Broken by ANY damage
- **Confusion**: Broken by ANY damage

Note: **Stun** is NOT broken by damage. **Poison**, **Blind**, **Silence**, **Bleeding**, **Curse** are NOT broken by damage.

---

## 3. Universal Resistance Formula

### Chance Formula (Pre-Renewal)

```
FinalChance% = BaseChance - (BaseChance * ResistStat / 100) + srcBaseLevel - tarBaseLevel - tarLUK
```

Where:
- `BaseChance` = the base % chance from the skill/card/weapon (e.g., Bash Lv10 = 30% stun)
- `ResistStat` = the target's relevant stat (VIT for stun, INT for sleep, etc.)
- `srcBaseLevel` = attacker's base level
- `tarBaseLevel` = target's base level
- `tarLUK` = target's LUK stat
- Result is clamped to **[5, 95]** -- there is always at minimum a 5% chance and at maximum a 95% chance

### Duration Formula (Pre-Renewal)

```
Duration(ms) = BaseDuration - (BaseDuration * ResistStat / 200) - 10 * tarLUK
```

Where:
- `BaseDuration` = the base duration in milliseconds
- Minimum duration is **1000ms** (1 second)
- Some effects (Stone Curse Phase 2) have a fixed duration regardless of stats

### Immunity Thresholds

| Immunity Type | Threshold | Notes |
|--------------|-----------|-------|
| Stat-based immunity | ResistStat >= ResistCap | e.g., 97 VIT = immune to Stun |
| LUK universal immunity | LUK >= 300 | Immune to ALL core status ailments |
| Boss protocol | `modeFlags.statusImmune = true` | Immune to almost all status ailments |
| Card/Equipment immunity | Specific cards | e.g., Marc Card = immune to Freeze |

### Special Cases

- **Curse**: Uses LUK as primary resist stat AND duration stat is VIT (unique split). 0 LUK targets cannot be cursed (special protection rule in rAthena).
- **Freeze**: Duration formula includes `+ 10 * srcLUK` (source LUK INCREASES freeze duration -- high-LUK Wizards freeze longer).
- **Stone Curse Phase 2**: Fixed 20,000ms duration regardless of stats once fully petrified.
- **Confusion**: Level difference is INVERTED: `- srcLv + tarLv` instead of `+ srcLv - tarLv`.
- **10+ Level Advantage**: When the source is 10+ levels above the target, some implementations bypass stat resistance entirely (notably for Poison from monsters).

---

## 4. Major Status Ailments (10 Core)

### 4.1 Stun

**rAthena SC**: `SC_STUN` (ID 2) | **OPT1**: 3 | **Icon**: Stars circling head

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, use skills, use items, sit, or pick up items. FLEE set to 0. HIT unaffected. |
| **Duration (base)** | 5,000ms |
| **Resist Stat (chance)** | VIT -- each point = 1% reduction in base chance |
| **Resist Stat (duration)** | VIT + LUK |
| **Immunity Threshold** | 97 VIT or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
FinalChance = BaseChance - (BaseChance * VIT / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Duration Formula**:
```
Duration(ms) = 5000 * (1 - VIT/100) - (srcLv - tarLv)/100 - tarLUK * 10
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Bash (Swordsman) Lv6+ | 5% per level above 5 (Lv6=5%, Lv10=25%) | Only at Lv6+ |
| Hammer Fall (Blacksmith) | 20% + 10% per level | AoE stun |
| Shield Charge (Crusader) | 20% + 5% per level | Knockback + stun |
| Shield Boomerang (Crusader) | Varies | Ranged stun |
| Storm Gust (Wizard) | 15-65% per hit (3+ hits = freeze, not stun) | Complex interaction |
| Charge Attack (Knight) | Varies by distance | Distance-based |
| Bowling Bash (Knight) | 5% per level | On knockback chain |
| Stun Attack (monster skill) | Varies | NPC_STUNATTACK |
| Stunner (weapon) | 10% per hit | Mace |
| Wrench (weapon) | 1% per hit | Tool |
| Orc Skeleton Card | 3% chance of stun when attacking | Weapon card |
| Savage Bebe Card | 5% chance of stun when attacking | Weapon card |
| Magnolia Card | 5% chance of stun when attacking | Weapon card |

**Cure Methods**:
- Status Recovery (Priest skill)
- Dispell (Sage skill)
- Natural expiry
- Battle Chant / Gospel (Paladin) -- random chance
- Note: NO item cure for Stun in pre-Renewal

**Special Mechanics**:
- Stun is the ONLY OPT1 status that is NOT broken by damage
- While stunned, FLEE is 0 -- all physical attacks will hit (assuming HIT > 0)
- Stun replaces other OPT1 statuses (Freeze, Sleep, Stone) when applied
- Equipment card resistance (Stalactic Golem: +20%, Orc Hero Card: 100% immunity)

---

### 4.2 Freeze

**rAthena SC**: `SC_FREEZE` (ID 1) | **OPT1**: 2 | **Icon**: Blue ice crystal encasing body

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, use skills, or use items. Hard DEF reduced by 50%. Hard MDEF increased by 25%. FLEE set to 0. **Element changes to Water Lv1.** |
| **Duration (base)** | 12,000ms |
| **Resist Stat (chance)** | Hard MDEF (equipment MDEF) -- each point = 1% reduction |
| **Resist Stat (duration)** | Hard MDEF (reduces) + source LUK (increases!) |
| **Immunity Threshold** | 300 LUK, or very high MDEF (100+ effectively) |
| **Broken by Damage** | YES -- any damage breaks freeze |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
FinalChance = BaseChance - (BaseChance * HardMDEF / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Duration Formula** (unique -- source LUK extends):
```
Duration(ms) = 12000 - (12000 * HardMDEF / 100) + 10 * srcLUK
Minimum 1000ms
```

Note: Source LUK ADDS to freeze duration. This is a critical difference from other statuses -- a high-LUK Wizard's Frost Diver will freeze targets longer.

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Frost Diver (Mage) | 38% + 12% per level (Lv10=158%, effectively 95% capped) | Primary freeze skill |
| Storm Gust (Wizard) | 15-65% per hit, 3 hits = guaranteed freeze | AoE freeze |
| Frost Nova (Wizard) | Lv-dependent freeze chance | AoE around caster |
| Cold Bolt | Does not freeze directly | Damage only |
| Freezing Trap (Hunter) | 100% on trigger | Trap-based |
| Frost Joker (Bard) | 12s freeze, MDEF-reduced chance | AoE around caster |
| Ice Arrow (ammo) | ~10% per hit | Status arrow |
| Stormy Knight Card | 2% chance when attacking | Weapon card |
| Marina Card | 5% chance when attacking | Weapon card |

**Cure Methods**:
- ANY damage (breaks freeze instantly)
- Status Recovery (Priest)
- Provoke (removes freeze as side effect in some implementations)
- Fire-element attacks (deal extra damage AND break freeze)
- Battle Chant / Gospel
- Note: NO direct item cure, but Fire damage items work

**Critical Mechanic -- Element Change**:
When frozen, the target's element becomes **Water Level 1** regardless of their original element. This means:
- Wind attacks deal **175%** damage (Wind is strong vs Water)
- Water attacks deal **25%** damage (Water resists Water)
- Fire attacks deal **150%** damage (Fire is strong vs Water)
- The element change persists for the entire duration of the freeze
- This is THE primary reason Wizard combos (Frost Diver -> Jupitel Thunder) are so effective

---

### 4.3 Stone Curse (Petrification)

**rAthena SC**: `SC_STONE` (ID 0) | **OPT1**: 1 (Phase 2) / 6 (Phase 1) | **Icon**: Grey stone encasing body

Stone Curse is unique among status ailments because it has TWO distinct phases.

#### Phase 1: Petrifying (SC_STONE with opt_val1 timer)

| Property | Value |
|----------|-------|
| **Duration** | 3-5 seconds (source-dependent, typically ~5s) |
| **Effects** | Can still MOVE. Cannot attack or use skills. CAN use items. |
| **Broken by Damage** | NO (damage does NOT cancel Phase 1) |
| **Transition** | Automatically transitions to Phase 2 when timer expires |
| **Items can cure** | Yes -- using a cure item during Phase 1 prevents full petrification |

#### Phase 2: Petrified (Full Stone)

| Property | Value |
|----------|-------|
| **Duration** | Fixed 20,000ms (NOT reduced by stats) |
| **Effects** | Cannot move, attack, use skills, or use items. Hard DEF reduced by 50%. Hard MDEF increased by 25%. FLEE set to 0. **Element changes to Earth Lv1.** HP drains 1% every 5 seconds (floor 25% HP). |
| **Broken by Damage** | YES -- any damage breaks full stone |
| **HP Drain** | 1% MaxHP every 5 seconds, stops at 25% MaxHP remaining |
| **Can Kill** | No (floor at 25% HP) |
| **Blocks HP Regen** | Effectively yes (HP is draining) |
| **Blocks SP Regen** | No |

**Chance Formula** (same as Freeze):
```
FinalChance = BaseChance - (BaseChance * HardMDEF / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Stone Curse (Mage/Sage) | 25% + 5% per level (Lv10=75%) | Primary stone skill, requires Red Gemstone |
| Medusa gaze (monster) | Varies | Monster-specific |
| Basilisk (monster) | Varies | Monster-specific |
| NPC_PETRIFYATTACK | Varies | Monster skill |
| Pest Card | 5% chance when attacking | Weapon card |

**Cure Methods**:
- ANY damage (breaks Phase 2 only)
- Status Recovery (Priest)
- Blessing (Priest) -- works on both phases
- Panacea, Royal Jelly (works during Phase 1 to prevent Phase 2)
- Battle Chant / Gospel

**Critical Mechanic -- Element Change**:
When fully petrified (Phase 2), the target's element becomes **Earth Level 1**:
- Fire attacks deal **150%** damage (Fire is strong vs Earth)
- Wind attacks deal **100%** damage (neutral)
- Water attacks deal **100%** damage (neutral)
- Earth attacks deal **100%** damage (Earth is neutral vs Earth in pre-Renewal)
- Poison attacks deal **125%** damage (Poison is strong vs Earth)

---

### 4.4 Sleep

**rAthena SC**: `SC_SLEEP` (ID 3) | **OPT1**: 4 (originally OPT2) | **Icon**: "ZZZ" above head

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, use skills, or use items. All physical attacks against sleeping target ALWAYS HIT (100% accuracy). Critical rate against sleeping targets is DOUBLED. |
| **Duration (base)** | 30,000ms |
| **Resist Stat (chance)** | INT -- each point = 1% reduction |
| **Resist Stat (duration)** | INT + LUK |
| **Immunity Threshold** | 97 INT or 300 LUK |
| **Broken by Damage** | YES -- any damage breaks sleep |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
FinalChance = BaseChance - (BaseChance * INT / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Duration Formula**:
```
Duration(ms) = 30000 - (30000 * INT / 200) - tarLUK * 10
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Lullaby (Bard ensemble) | 5% per tick (~12s freeze with MDEF reduction) | AoE ground effect |
| NPC_SLEEPATTACK | Varies | Monster skill |
| Nightmare Card | Immunity (equip) | Headgear card |
| Plankton Card | 5% chance when attacking | Weapon card |
| Sleep Arrow (ammo) | ~10% per hit | Status arrow |
| Sandman (Hunter trap) | 100% on trigger | AoE sleep trap |

**Cure Methods**:
- ANY damage (even 1 damage breaks sleep)
- Dispell (Sage)
- Battle Chant / Gospel
- Natural expiry
- Alarm Clock item (some servers)
- Note: Panacea does NOT cure Sleep in classic pre-Renewal

**Special Mechanics**:
- Attacks against sleeping targets have 100% hit rate (FLEE is ignored)
- Critical chance is doubled against sleeping targets
- This makes Sleep extremely dangerous in PvP -- an Assassin can crit a sleeping target reliably

---

### 4.5 Poison

**rAthena SC**: `SC_POISON` (ID 4) | **OPT2**: 0x0001 | **Icon**: Green bubbles

| Property | Value |
|----------|-------|
| **Effects** | Hard DEF reduced by 25%. HP drains continuously. SP regeneration disabled. Can still move, attack, and use skills. |
| **Duration (base)** | 60,000ms (players), 30,000ms (monsters) |
| **Resist Stat (chance)** | VIT -- each point = 1% reduction |
| **Resist Stat (duration)** | VIT + LUK |
| **Immunity Threshold** | 97 VIT or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | NO (HP cannot drop below 25% from poison drain) |
| **Blocks HP Regen** | No (poison drain overrides but doesn't technically block) |
| **Blocks SP Regen** | YES |

**HP Drain Formula**:
```
Drain per second = floor(MaxHP * 0.015) + 2
Cannot reduce HP below 25% of MaxHP
```

**Duration Formula (Players)**:
```
Duration(ms) = 60000 - (45000 * VIT / 100) - 100 * tarLUK
Minimum 1000ms
```

**Duration Formula (Monsters)**:
```
Duration(ms) = 30000 - (20000 * VIT / 100)
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Envenom (Thief) | 15% + 5% per level (Lv10=65%) | +ATK bonus |
| Enchant Poison (Assassin) | Gives poison-element attacks | Adds poison proc |
| Venom Dust (Assassin) | 50% per tick | Ground AoE |
| Poison React (Assassin) | Counter + poison proc | Reactive |
| Venom Splasher (Assassin Cross) | 100% | Timed explosion |
| Poison Arrow (ammo) | ~10% per hit | Status arrow |
| Poison-property monsters | Varies by monster | Auto-attack proc |
| Anacondaques Card | 5% chance when attacking | Weapon card |
| Poison Spore Card | 5% chance when attacked | Armor card |

**Cure Methods**:
- Green Herb (item)
- Green Potion (item)
- Panacea (item)
- Royal Jelly (item)
- Detoxify (Thief skill)
- Cure (Priest skill)
- Slow Poison (Priest skill) -- suspends but does not remove
- Battle Chant / Gospel

**Special: 10+ Level Override**:
When the attacker is 10 or more levels above the target, some poison sources bypass stat resistance entirely. This primarily applies to monster-inflicted poison.

---

### 4.6 Deadly Poison (SC_DPOISON)

**rAthena SC**: `SC_DPOISON` (ID 10) | **OPT2**: 0x0080 | **Icon**: Purple bubbles (darker than normal poison)

| Property | Value |
|----------|-------|
| **Effects** | Same as Poison but with faster, more severe HP drain. Hard DEF reduced by 25%. SP regeneration disabled. Unlike normal poison, the HP drain is much more aggressive. |
| **Duration (base)** | Same as Poison (skill-dependent) |
| **Resist Stat** | Same as Poison (VIT-based) |
| **Immunity Threshold** | 97 VIT or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | YES (unlike normal Poison, Deadly Poison CAN kill) |
| **Blocks HP Regen** | Yes |
| **Blocks SP Regen** | YES |

**HP Drain Formula**:
```
Drain per second = floor(MaxHP * 0.02) + floor(MaxHP / 100) * 3
No minimum HP floor -- can kill the target
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Enchant Deadly Poison (Assassin Cross) | Applied to self, affects targets on hit | Self-buff that adds poison proc |
| NPC_POISON / NPC_DPOISON | Varies | Monster-specific |
| Certain MVPs | Varies | Boss attacks |

**Cure Methods**:
- Same as regular Poison (Green Herb, Panacea, etc.)
- Slow Poison does NOT work on Deadly Poison
- Antidote items

**Key Difference from Poison**:
- Normal Poison floors at 25% HP and cannot kill
- Deadly Poison has NO floor and CAN kill
- Deadly Poison drains significantly faster
- Both share the same DEF reduction and SP regen block

---

### 4.7 Blind (Darkness)

**rAthena SC**: `SC_BLIND` (ID 8) | **OPT2**: 0x0010 | **Icon**: Darkness overlay on screen

| Property | Value |
|----------|-------|
| **Effects** | HIT reduced by 25%. FLEE reduced by 25%. Player's visible screen range is dramatically reduced (fog of war effect). |
| **Duration (base)** | 30,000ms |
| **Resist Stat (chance)** | Average of INT and VIT: floor((INT + VIT) / 2) |
| **Resist Stat (duration)** | Same composite + LUK |
| **Immunity Threshold** | 193 combined (INT+VIT) or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
ResistStat = floor((INT + VIT) / 2)
FinalChance = BaseChance - (BaseChance * ResistStat / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Duration Formula**:
```
Duration(ms) = 30000 - (30000 * ResistStat / 200) - tarLUK * 10
Minimum 15000ms (Blind has a higher minimum duration than most statuses!)
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Sand Attack (Rogue) | High chance | Ground-targeted |
| NPC_BLINDATTACK | Varies | Monster skill |
| Curse Arrow (ammo) | 10% per hit (inflicts Curse, not Blind -- note distinction) | Naming confusion |
| Dark-element monster attacks | Low-moderate % | Auto-attack proc |
| Familiar Card | 5% chance when attacking | Weapon card |

**Cure Methods**:
- Green Potion (item)
- Panacea (item)
- Royal Jelly (item)
- Cure (Priest skill)
- Battle Chant / Gospel
- Note: Green Herb does NOT cure Blind (only Panacea/Royal Jelly/Green Potion work)

**Special Mechanics**:
- The screen darkening effect is purely cosmetic on the client but the HIT/FLEE reduction is real
- Minimum duration of 15 seconds even with max stats (unlike most other ailments)
- On monsters, Blind reduces their detection/aggro range

---

### 4.8 Silence

**rAthena SC**: `SC_SILENCE` (ID 6) | **OPT2**: 0x0004 | **Icon**: "..." speech bubble

| Property | Value |
|----------|-------|
| **Effects** | Cannot use any active skills. Can still move, auto-attack, and use items. |
| **Duration (base)** | 30,000ms |
| **Resist Stat (chance)** | VIT -- each point = 1% reduction |
| **Resist Stat (duration)** | VIT + LUK |
| **Immunity Threshold** | 97 VIT or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
FinalChance = BaseChance - (BaseChance * VIT / 100) + srcLv - tarLv - tarLUK
Clamped to [5, 95]
```

**Duration Formula**:
```
Duration(ms) = 30000 * (100 - VIT) / 100
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Lex Divina (Priest) | 100% (no resistance check) | Toggle skill -- re-cast to remove |
| NPC_SILENCEATTACK | Varies | Monster skill |
| Classical Pluck (Bard ensemble) | AoE silence zone | Ground effect |
| Pang Voice (Bard/Dancer) | Formula-based rate | Single target |
| Marduk Card | 100% immunity (equip) | Headgear card |
| Stainer Card | 5% chance when attacked | Armor card |

**Cure Methods**:
- Green Potion (item)
- Panacea (item)
- Royal Jelly (item)
- Cure (Priest skill)
- Lex Divina (toggles -- cast again to remove)
- Battle Chant / Gospel
- Note: Green Herb does NOT cure Silence

**Special Mechanics**:
- Silence only prevents active skill use -- passives and auto-attack still work
- Silence does NOT cancel active performances (Bard songs/Dancer dances)
- This makes it particularly devastating to pure caster classes (Wizard, Priest)
- Monsters under Silence cannot use their skills but can still auto-attack

---

### 4.9 Confusion (Chaos)

**rAthena SC**: `SC_CONFUSION` (ID 7) | **OPT2**: 0x0008 | **Icon**: Stars/spirals above head

| Property | Value |
|----------|-------|
| **Effects** | Movement direction is randomized. When the player inputs a direction, the character moves in a random direction instead. Can still attack and use skills (but movement is unreliable). |
| **Duration (base)** | 30,000ms |
| **Resist Stat (chance)** | Average of STR and INT: floor((STR + INT) / 2) |
| **Resist Stat (duration)** | Same composite + LUK |
| **Immunity Threshold** | 193 combined (STR+INT) or 300 LUK |
| **Broken by Damage** | YES -- any damage breaks confusion |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula** (INVERTED level difference!):
```
ResistStat = floor((STR + INT) / 2)
FinalChance = BaseChance - (BaseChance * ResistStat / 100) - srcLv + tarLv + tarLUK
NOTE: Level difference is INVERTED compared to other statuses!
```

**Duration Formula**:
```
Duration(ms) = 30000 - (30000 * ResistStat / 200) - tarLUK * 10
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| NPC_CONFUSEATTACK | Varies | Monster skill |
| Scream (Dancer) | AoE confusion | Area around caster |
| Eternal Chaos (Bard/Dancer ensemble) | AoE zone | Ground effect (sets target DEF to 0) |
| Certain monster skills | Varies | Boss attacks |

**Cure Methods**:
- ANY damage (breaks confusion)
- Cure (Priest skill)
- Panacea (item)
- Royal Jelly (item)
- Battle Chant / Gospel

**Special Mechanics**:
- Only affects movement input -- skills and attacks still target correctly
- Client-side implementation randomizes the direction the character walks
- In PvE, confused monsters will wander randomly instead of chasing their target
- Broken easily by any damage, making it a weak CC in practice

---

### 4.10 Bleeding

**rAthena SC**: `SC_BLEEDING` (ID 9) | **OPT2**: 0x0040 | **Icon**: Blood drops

| Property | Value |
|----------|-------|
| **Effects** | HP drains over time (slower than Poison but **CAN KILL**). All natural HP and SP regeneration disabled. Persists through relog in some implementations. |
| **Duration (base)** | 120,000ms (2 minutes -- longest base duration) |
| **Resist Stat (chance)** | VIT -- each point = 1% reduction |
| **Resist Stat (duration)** | VIT + LUK |
| **Immunity Threshold** | 97 VIT or 300 LUK |
| **Broken by Damage** | NO |
| **Can Kill** | YES (HP can reach 0) |
| **Blocks HP Regen** | YES |
| **Blocks SP Regen** | YES |

**HP Drain Formula**:
```
Drain every 4 seconds = floor(MaxHP * 0.02)
No minimum HP floor -- can reduce to 0 and kill
```

**Duration Formula**:
```
Duration(ms) = 120000 - (120000 * VIT / 100) - tarLUK * 10
Minimum 1000ms
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| Clashing Spiral / Spiral Pierce (Lord Knight) | Moderate % | Physical skill |
| Acid Terror (Alchemist) | Has bleeding proc | Chemical skill |
| NPC_BLEEDING | Varies | Monster skill |
| Certain MVP monster attacks | Varies | Boss attacks |
| Some 2nd Trans class skills | Varies | Renewal-adjacent |

**Cure Methods**:
- Death (dying resets bleeding -- but that's hardly ideal)
- Battle Chant / Gospel (random chance)
- Dispell (Sage skill)
- Meat/Banana Juice (limited server implementations)
- Note: VERY few cures exist for Bleeding -- it is intentionally hard to cure

**Special Mechanics**:
- Bleeding is the most dangerous DoT because it CAN kill and blocks ALL regen
- Combined with Poison, a target has no HP regen, no SP regen, and two DoTs running
- The 2-minute base duration makes it extremely punishing even with moderate VIT
- Few skills and almost no items cure bleeding in pre-Renewal

---

### 4.11 Curse

**rAthena SC**: `SC_CURSE` (ID 5) | **OPT2**: 0x0002 | **Icon**: Dark aura / skull

| Property | Value |
|----------|-------|
| **Effects** | ATK reduced by 25%. LUK becomes 0. Movement speed drastically reduced (~10% of normal). |
| **Duration (base)** | 30,000ms |
| **Resist Stat (chance)** | LUK (unique -- LUK is primary resist for chance) |
| **Resist Stat (duration)** | VIT (unique -- VIT reduces duration, not LUK) |
| **Immunity Threshold** | 97 LUK (for chance) or 100 VIT (for duration reduction) |
| **Broken by Damage** | NO |
| **Can Kill** | No |
| **Blocks HP Regen** | No |
| **Blocks SP Regen** | No |

**Chance Formula**:
```
FinalChance = BaseChance - (BaseChance * LUK / 100) + srcLv - tarLUK
NOTE: Uses srcLv - tarLUK (not tarLv), and LUK is the primary resist
```

**Duration Formula**:
```
Duration(ms) = 30000 * (100 - VIT) / 100 - tarLUK * 10
Minimum 1000ms
NOTE: VIT reduces duration (not LUK)
```

**Infliction Sources**:
| Source | Base Chance | Notes |
|--------|------------|-------|
| NPC_CURSEATTACK | Varies | Monster skill (Undead monsters) |
| Curse-property monster attacks | Low % | Auto-attack from Dark/Undead monsters |
| Curse Arrow (ammo) | ~10% per hit | Status arrow |
| Magnolia Card | 5% curse chance when attacking | Weapon card |
| Munak Card | 5% curse chance when attacking | Weapon card |
| Skeleton Card | 5% curse chance when attacking | Weapon card |

**Cure Methods**:
- Blessing (Priest skill) -- primary cure
- Holy Water (item)
- Panacea (item)
- Royal Jelly (item)
- Yggdrasil Dust (rare item)
- Battle Chant / Gospel

**Special Mechanics**:
- Curse sets LUK to 0 -- this removes all LUK-based bonuses (crit rate, perfect dodge, status resistance)
- The ~90% movement speed reduction is the most severe slow in the game
- LUK 0 targets CANNOT be cursed (special protection in rAthena -- divide by zero prevention)
- Blessing on an Undead-property target applies Curse instead of the normal blessing effect
- Curse + Blind is a devastating combo (slow + reduced HIT/FLEE + no crit from 0 LUK)

---

## 5. Secondary Debuffs (Skill-Based)

These are implemented as buff-system entries with `category: 'debuff'` rather than core status ailments. They have skill-specific application logic rather than the universal resistance formula.

### 5.1 Provoke

| Property | Value |
|----------|-------|
| **Skill** | Provoke (Swordsman, ID 6) |
| **Effects** | Target ATK +2% per level (up to +20% at Lv10), DEF -5% per level (up to -50% at Lv10). Forced aggro on caster for monsters. |
| **Duration** | 30 seconds |
| **Resistance** | Fails on Undead-property targets. Level-based success rate. |
| **Cure** | Natural expiry, Dispell |
| **Boss interaction** | Works on bosses (forces aggro, applies stat changes) |
| **Notes** | Can remove Freeze status when applied. Debuff+buff hybrid (ATK up but DEF down). |

### 5.2 Lex Aeterna

| Property | Value |
|----------|-------|
| **Skill** | Lex Aeterna (Priest, ID 78) |
| **Effects** | Next incoming damage is DOUBLED. Consumed on the first damage instance. |
| **Duration** | Until damage is received (no time limit) |
| **Resistance** | Cannot be applied to frozen/stone targets. |
| **Cure** | Taking any damage (consumed), Dispell |
| **Boss interaction** | Works on bosses |
| **Notes** | Only the first HIT of a multi-hit skill is doubled. Healing/misses/Shield Reflect do not consume. Guard/Parry/Kaupe block consumption even if no damage. |

### 5.3 Lex Divina

| Property | Value |
|----------|-------|
| **Skill** | Lex Divina (Priest, ID 76) |
| **Effects** | Silence -- cannot use active skills |
| **Duration** | 30s + 10s per level (Lv5 = 80s, Lv6 = 60s -- unique scaling) |
| **Resistance** | No resistance check -- always applies |
| **Cure** | Casting Lex Divina again on the same target removes it (toggle). Green Potion, Panacea. |
| **Boss interaction** | Does NOT work on boss-protocol monsters |
| **Notes** | At Lv5 the duration peaks (80s). Lv6-10 decrease duration but reduce SP cost. Toggle mechanic makes it unique. |

### 5.4 Signum Crucis

| Property | Value |
|----------|-------|
| **Skill** | Signum Crucis (Acolyte, ID 32) |
| **Effects** | Reduces all affected enemies' DEF by 14% + 2% per level (Lv10 = 34%) |
| **Duration** | Until death or zone change |
| **Resistance** | Only affects Undead and Demon race monsters. Chance-based. |
| **Cure** | Monster death, zone change |
| **Boss interaction** | Works on boss Undead/Demon race monsters |

### 5.5 Decrease AGI

| Property | Value |
|----------|-------|
| **Skill** | Decrease AGI (Acolyte, ID 30) |
| **Effects** | AGI reduced by (3 + level) points. Movement speed reduced. |
| **Duration** | 40s + 2s per level (Lv10 = 60s) |
| **Resistance** | INT-based success chance. MDEF provides resistance. |
| **Cure** | Increase AGI (replaces), Dispell, natural expiry |
| **Boss interaction** | Reduced effect on bosses |
| **Notes** | Removes Increase AGI, Two Hand Quicken, Adrenaline Rush, Wind Walker when applied |

### 5.6 Quagmire

| Property | Value |
|----------|-------|
| **Skill** | Quagmire (Wizard, ID 92) |
| **Effects** | AGI reduced by 10-50 (5+5*lv). DEX reduced by 10-50 (5+5*lv). Movement speed reduced by 50%. |
| **Duration** | 5s + 5s per level (Lv5 = 30s) |
| **Resistance** | No stat resistance -- always applies in AoE |
| **Cure** | Leaving the AoE area (reapplies on re-entry). Dispell. |
| **Boss interaction** | Works on bosses (one of the few debuffs that do!) |
| **Notes** | Removes: Increase AGI, Two Hand Quicken, Spear Quicken, Adrenaline Rush, Wind Walker, One Hand Quicken. Ground-targeted AoE -- persists as zone. Stat reduction cap: 50% for monsters, 25% for players. |

### 5.7 Hallucination

| Property | Value |
|----------|-------|
| **Skill** | Applied by Quagmire in some implementations, or by monster skills |
| **Effects** | Screen goes wavy. Fake damage numbers appear (random large numbers displayed for all damage around the player). |
| **Duration** | Varies (30-60s typical) |
| **Resistance** | INT-based in some implementations |
| **Cure** | Natural expiry. Panacea. Royal Jelly. |
| **Boss interaction** | N/A (primarily a player-targeted effect) |
| **Notes** | Primarily a visual/psychological debuff. No actual stat changes. All displayed damage numbers are randomized and fake. Rare in pre-Renewal outside of specific monsters. |

### 5.8 Coma

| Property | Value |
|----------|-------|
| **Skill** | Card effect / Monster skill |
| **Effects** | Reduces HP to 1 and SP to 1 instantly. |
| **Duration** | Instant (no duration) |
| **Resistance** | LUK reduces chance. High FLEE can dodge the physical attack that procs it. |
| **Cure** | N/A (instant effect -- heal afterwards) |
| **Boss interaction** | Does NOT work on boss-protocol monsters |
| **Notes** | Extremely rare. Primarily from cards (e.g., Turtle General Card: 1% coma chance on attack). Not a persistent status -- it's an instant HP/SP reduction. |

**Coma Sources**:
| Source | Chance | Notes |
|--------|--------|-------|
| Turtle General Card | 1% on physical attack | Weapon card |
| Some MVP monster skills | Varies | Boss attacks on players |
| NPC_COMA | Fixed rate | Monster skill |

### 5.9 Strip / Divest Effects

Rogue/Stalker skills that forcibly remove equipment, applying stat penalties:

| Skill | What It Strips | Monster Effect | Duration | Resist |
|-------|---------------|---------------|----------|--------|
| Divest Weapon (ID 215) | Weapon | -25% ATK | DEX-based | Target DEX |
| Divest Shield (ID 216) | Shield | -15% DEF | DEX-based | Target DEX |
| Divest Armor (ID 217) | Armor | -40% VIT DEF | DEX-based | Target DEX |
| Divest Helm (ID 218) | Headgear | -15% INT/DEX | DEX-based | Target DEX |
| Full Divestment (Stalker) | All 4 | Combined | DEX-based | Target DEX |

**Key Mechanics**:
- Duration formula: Base 13s, reduced by target DEX
- Cannot strip equipment protected by Chemical Protection (Alchemist)
- Works on boss-protocol monsters (one of the few debuffs that works on bosses)
- On monsters: applies stat reduction since monsters cannot "equip" items
- Strip effects are NOT removed by Dispell

### 5.10 Ankle Snare (Root)

| Property | Value |
|----------|-------|
| **Skill** | Ankle Snare (Hunter, ID 117) |
| **Effects** | Target is rooted (cannot move). Can still attack and use skills. |
| **Duration** | Base 4s per level, reduced by target AGI: Duration = BaseDuration - AGI/10 (seconds) |
| **Duration (boss)** | Reduced to 1/5 normal duration |
| **Resistance** | No resistance check -- always applies when triggered. AGI only reduces duration. |
| **Cure** | Natural expiry. Teleportation skills (Fly Wing, Warp Portal). |
| **Boss interaction** | Works but duration is 1/5 normal |
| **Minimum Duration** | 3000ms + 30ms * srcBaseLv (rAthena) |
| **Notes** | Trap-based -- must step on the trap. Destroyed after use. |

### 5.11 Close Confine

| Property | Value |
|----------|-------|
| **Skill** | Close Confine (Rogue quest skill, ID 1718) |
| **Effects** | Both caster and target are rooted. Neither can move. Caster gains +10 FLEE. |
| **Duration** | 10 seconds |
| **Resistance** | Boss immune |
| **Cure** | Duration expiry. Death of either party. Knockback. Teleport/Fly Wing. Hiding. |
| **Break conditions** | 8 break conditions: knockback, death, teleport, Fly Wing, Butterfly Wing, hiding, distance, expiry |

### 5.12 Spider Web

| Property | Value |
|----------|-------|
| **Skill** | Spider Web (Sage quest skill, ID 1422) |
| **Effects** | Target is rooted and takes double Fire damage. FLEE reduced by 50%. |
| **Duration** | 8 seconds |
| **Resistance** | Boss immune |
| **Cure** | Natural expiry. Fire damage (breaks web AND deals double). Any AoE attack to the web cell. |
| **Notes** | Sets target to act as if Fire Lv1 property for incoming fire attacks. Creates a "web" cell on the ground. |

### 5.13 Enchant Deadly Poison (EDP Debuff)

| Property | Value |
|----------|-------|
| **Skill** | Enchant Deadly Poison (Assassin Cross, ID 378) |
| **Type** | Self-buff that makes attacks deal poison damage to targets |
| **Effects** | +400% weapon ATK (pre-Renewal), attacks can inflict Deadly Poison on targets |
| **Duration** | 40s + 20s per level (Lv5 = 140s) |
| **Cure** | Natural expiry. Cannot be Dispelled. |
| **Notes** | The EDP buff on the Assassin Cross is NOT a debuff. But the Deadly Poison status it inflicts on targets IS (see Section 4.6). |

### 5.14 Safety Wall / Pneuma Expiry

Not debuffs themselves but their ABSENCE is notable. When these protective buffs expire, the player loses protection:
- Safety Wall: Blocks melee attacks (limited hit count)
- Pneuma: Blocks all ranged attacks

### 5.15 Ensemble Aftermath

| Property | Value |
|----------|-------|
| **Skill** | Aftermath of ending a Bard/Dancer ensemble |
| **Effects** | Movement speed penalty and skill use delay after ensemble ends |
| **Duration** | 15-30 seconds (varies by implementation) |
| **Cure** | Natural expiry only |
| **Notes** | Both the Bard and Dancer receive this penalty when an ensemble ends or is interrupted |

---

## 6. Status Effect Resistance Formulas (Detailed)

### 6.1 VIT-Based Resistance

VIT is the primary resistance stat for the most common status ailments.

**Affects**: Stun, Poison, Silence, Bleeding

**Formula**:
```
ChanceReduction = BaseChance * VIT / 100
DurationReduction = BaseDuration * VIT / 200

Practical example (Stun, 50% base chance, 60 VIT target):
FinalChance = 50 - (50 * 60/100) + srcLv - tarLv - tarLUK
           = 50 - 30 + levelDiff - LUK
           = 20 + levelDiff - LUK
```

**Immunity**: 97 VIT grants immunity to Stun, Poison, Silence, and Bleeding.

**VIT also reduces Curse DURATION** (but not chance -- LUK does that for Curse).

### 6.2 INT-Based Resistance

**Affects**: Sleep

**Formula**:
```
ChanceReduction = BaseChance * INT / 100
DurationReduction = BaseDuration * INT / 200
```

**Immunity**: 97 INT grants immunity to Sleep.

### 6.3 LUK-Based Resistance

LUK has two roles in the status effect system:

**Universal minor resistance**: LUK provides a flat chance reduction and duration reduction to ALL status ailments:
```
ChanceReduction(LUK) = tarLUK (flat subtraction from chance)
DurationReduction(LUK) = 10 * tarLUK (ms subtracted from duration)
```

**Primary resistance for Curse**: LUK is the main resist stat for Curse chance:
```
CurseChanceReduction = BaseChance * LUK / 100
```

**Immunity**:
- 300 LUK = immune to ALL core status ailments
- 97 LUK = immune to Curse specifically
- 0 LUK = cannot be Cursed (special protection)

### 6.4 MDEF-Based Resistance

Hard MDEF (from equipment, not INT-based soft MDEF) provides resistance to magic-originated ailments.

**Affects**: Freeze, Stone Curse

**Formula**:
```
ChanceReduction = BaseChance * HardMDEF / 100
Each point of hard MDEF = 1% resistance to Freeze and Stone Curse
```

**No cap**: Unlike VIT/INT (capped at 97), MDEF resistance has no hard cap, but equipment MDEF rarely exceeds 50-60 in pre-Renewal.

### 6.5 Composite Stat Resistance

**Blind**: floor((INT + VIT) / 2)
- Immunity at 193 combined (e.g., 97 VIT + 96 INT, or 97 INT + 96 VIT)

**Confusion**: floor((STR + INT) / 2)
- Immunity at 193 combined (e.g., 97 STR + 96 INT)
- Note: Confusion's level difference is INVERTED (higher level target = more resistant)

---

## 7. Boss Protocol Status Immunity

Monsters with the Boss Protocol flag (`modeFlags.statusImmune = true`) have special interactions with status effects.

### Completely Immune

Boss-protocol monsters are **fully immune** to:
- Stun (SC_STUN)
- Freeze (SC_FREEZE)
- Stone Curse (SC_STONE)
- Sleep (SC_SLEEP)
- Poison (SC_POISON)
- Deadly Poison (SC_DPOISON)
- Silence (SC_SILENCE)
- Blind (SC_BLIND)
- Curse (SC_CURSE)
- Confusion (SC_CONFUSION)
- Bleeding (SC_BLEEDING)
- Coma (instant HP/SP reduction)
- Close Confine
- Spider Web
- Fear / Deep Sleep (Renewal-only, but immune regardless)

### Partially Effective (Reduced Duration)

Some effects work on bosses but with reduced effectiveness:
- **Ankle Snare**: Duration reduced to 1/5 of normal
- **Quagmire**: Full effect (no reduction) -- one of the few that works normally
- **Provoke**: Works (forces aggro + ATK/DEF changes)
- **Decrease AGI**: Reduced success rate against bosses
- **Lex Aeterna**: Works (double damage on next hit)
- **Signum Crucis**: Works on Undead/Demon race bosses

### Always Works on Bosses

- **Strip/Divest skills**: Always work (no boss immunity)
- **Quagmire**: Always works (stat reduction + slow)
- **Lex Aeterna**: Always works (double damage)
- **Provoke**: Always works (aggro + stat changes)
- **Dispell**: Always works (removes buffs)

### Detection of Hidden Players

Boss-protocol monsters can detect players using:
- Hiding
- Cloaking
- Chase Walk
- Any similar invisibility skill

---

## 8. Frozen = Water Lv1, Stone = Earth Lv1

Two status ailments change the target's element, creating critical combat interactions:

### Freeze -> Water Level 1

When a target is Frozen, their armor element becomes Water Level 1. Element damage modifiers:

| Attacking Element | Damage vs Water Lv1 | Practical Impact |
|------------------|---------------------|------------------|
| Neutral | 100% | Normal |
| Water | 25% | Strong resistance |
| Earth | 100% | Normal |
| Fire | 150% | Bonus damage |
| Wind | 175% | **Maximum bonus** |
| Poison | 100% | Normal |
| Holy | 75% | Resisted |
| Shadow/Dark | 75% | Resisted |
| Ghost | 100% | Normal |
| Undead | 100% | Normal |

**Key combos**:
- Frost Diver (Freeze) -> Jupitel Thunder (Wind) = 175% damage on each hit
- Storm Gust (Freeze) -> Lord of Vermilion (Wind) = 175% damage per tick
- Any Freeze -> Wind endowed melee = 175% per hit

### Stone Curse -> Earth Level 1

When a target is fully Petrified, their armor element becomes Earth Level 1:

| Attacking Element | Damage vs Earth Lv1 | Practical Impact |
|------------------|---------------------|------------------|
| Neutral | 100% | Normal |
| Water | 100% | Normal |
| Earth | 100% | Neutral (same) |
| Fire | 150% | Bonus damage |
| Wind | 100% | Normal |
| Poison | 125% | Moderate bonus |
| Holy | 75% | Resisted |
| Shadow/Dark | 75% | Resisted |
| Ghost | 100% | Normal |
| Undead | 100% | Normal |

**Key combos**:
- Stone Curse -> Fire Bolt/Fire Ball = 150% damage
- Stone Curse -> Meteor Storm = 150% damage per hit
- Stone Curse -> Poison-element attack = 125% damage

### Implementation Notes

- The element override persists ONLY while the status is active
- When the status is broken (by damage or expiry), the target's original element is restored
- The element change affects ALL incoming damage, not just from the caster
- This means allies can capitalize on a Wizard's freeze/stone combos
- **Both Freeze and Stone also reduce DEF by 50%**, amplifying the combo damage further

---

## 9. Status Immunity Items & Cards

### Full Immunity Cards (100% Protection)

| Card | Immunity | Slot | Notes |
|------|----------|------|-------|
| Orc Hero Card | Stun | Headgear | +VIT 3 bonus |
| Marc Card | Freeze | Armor | Changes armor to Water Lv1 (Freeze doesn't apply to Water armor) |
| Evil Druid Card | Freeze, Stone | Armor | Changes armor to Undead Lv1 (both immunities + Heal damages) |
| Nightmare Card | Sleep | Headgear | +AGI 1 bonus |
| Marduk Card | Silence | Headgear | No additional bonus |
| Argiope Card | Poison (property) | Armor | Changes armor to Poison Lv1 |
| Bathory Card | Curse (Undead property) | Armor | Changes armor to Shadow Lv1 |
| Green Ferus Card | Stone Curse | Armor (in some implementations) | Varies by server |

### Resistance Cards (Partial Protection)

| Card | Resistance | Slot |
|------|-----------|------|
| Stalactic Golem Card | +20% Stun resist | Headgear |
| Gemini-S58 Card | +30% Stun/Silence resist (80+ base VIT) | Headgear |
| Coco Card | +20% Silence resist | Headgear |
| Flame Skull Card | +30% Stun/Curse/Blind resist | Headgear |
| Deviling Card | +50% vs Neutral, -50% vs all other elements | Garment |

### Cure Items

| Item | Cures | Acquisition |
|------|-------|-------------|
| Green Herb | Poison | NPC shops, monster drops |
| Green Potion | Poison, Silence, Blind | Crafted from Green Herb + Empty Bottle |
| Panacea | Poison, Silence, Blind, Confusion, Curse | NPC shops (expensive) |
| Royal Jelly | ALL status ailments + HP/SP restore | Monster drops (rare) |
| Holy Water | Curse | Aqua Benedicta (Acolyte) or NPC |
| Yggdrasil Dust | Curse | Rare monster drops |
| Yggdrasil Leaf | Resurrects dead players | Rare (does not cure status) |

---

## 10. Status Effects from Weapons & Arrows

### Status Arrows

Arrows with innate status effects proc on each ranged physical attack:

| Arrow Type | Status Inflicted | Proc Rate | Base Chance |
|-----------|-----------------|----------|-------------|
| Stun Arrow | Stun | ~5-10% | Subject to VIT resist |
| Curse Arrow | Curse | ~5-10% | Subject to LUK resist |
| Sleep Arrow | Sleep | ~5-10% | Subject to INT resist |
| Silence Arrow | Silence | ~5-10% | Subject to VIT resist |
| Poison Arrow | Poison | ~5-10% | Subject to VIT resist |
| Ice Arrow (Freezing Arrow) | Freeze | ~5-10% | Subject to MDEF resist |
| Flash Arrow | Blind | ~5-10% | Subject to composite resist |

### Weapon Status Procs

Certain weapons have built-in status proc chances:

| Weapon | Status | Proc Rate |
|--------|--------|----------|
| Stunner (Mace) | Stun | 10% per hit |
| Wrench (etc.) | Stun | 1% per hit |
| Ice Falchion | Freeze | 5% per hit |
| Poison Knife | Poison | Enchant Poison-like |

### Card-Based Status Procs (When Attacking)

Cards slotted in weapons can add status chances:

| Card | Status | Chance | Notes |
|------|--------|--------|-------|
| Savage Bebe Card | Stun | 5% | Weapon card |
| Magnolia Card | Stun | 5% | Weapon card |
| Orc Skeleton Card | Stun | 3% | Weapon card |
| Marina Card | Freeze | 5% | Weapon card |
| Stormy Knight Card | Freeze | 2% | Weapon card |
| Plankton Card | Sleep | 5% | Weapon card |
| Pest Card | Stone Curse | 5% | Weapon card |
| Familiar Card | Blind | 5% | Weapon card |
| Anacondaques Card | Poison | 5% | Weapon card |
| Requiem Card | Confusion | 5% | Weapon card |
| Munak Card | Curse | 5% | Weapon card |
| Skeleton Card | Curse | 5% | Weapon card |
| Arclouse Card | Curse | 10% | Weapon card |

### Card-Based Status Procs (When Attacked -- Armor/Shield)

| Card | Status | Chance | Notes |
|------|--------|--------|-------|
| Poison Spore Card | Poison | 5% when receiving physical attack | Armor card |
| Stainer Card | Silence | 5% when receiving physical attack | Armor card |
| Venomous Card | Poison | 30% to self + 30% to attacker | Special |

### Boss Immunity Override

All status procs from weapons, arrows, and cards are subject to boss immunity. If the target has boss protocol, the status will NOT apply regardless of proc chance.

**Exception**: When the source is 10+ levels above the target, some implementations reduce or ignore the target's stat-based resistance (but boss immunity still applies).

---

## 11. Cure Methods Reference Table

| Status | Green Herb | Green Potion | Panacea | Royal Jelly | Holy Water | Cure (Priest) | Status Recovery | Blessing | Detoxify | Dispell | Battle Chant |
|--------|-----------|-------------|---------|-------------|-----------|--------------|----------------|----------|----------|---------|-------------|
| Stun | - | - | - | - | - | - | YES | - | - | YES | YES |
| Freeze | - | - | - | - | - | - | YES | - | - | - | YES |
| Stone | - | - | - | - | - | - | YES | YES | - | - | YES |
| Sleep | - | - | - | - | - | - | - | - | - | YES | YES |
| Poison | YES | YES | YES | YES | - | YES | - | - | YES | - | YES |
| Deadly Poison | YES | YES | YES | YES | - | YES | - | - | - | - | YES |
| Blind | - | YES | YES | YES | - | YES | - | - | - | - | YES |
| Silence | - | YES | YES | YES | - | YES | - | - | - | - | YES |
| Confusion | - | - | YES | YES | - | YES | - | - | - | - | YES |
| Bleeding | - | - | - | - | - | - | - | - | - | YES | YES |
| Curse | - | - | YES | YES | YES | - | - | YES | - | - | YES |

Legend: YES = cures this status, `-` = does not cure

**Additional cure notes**:
- **Damage**: Breaks Freeze, Stone (Phase 2), Sleep, Confusion
- **Lex Divina toggle**: Re-casting Lex Divina removes its own Silence
- **Provoke**: Removes Freeze in some implementations
- **Slow Poison**: Suspends Poison damage but does not remove the status
- **Yggdrasil Dust**: Cures Curse (same as Holy Water)
- **Status Recovery (Priest)**: Cures Stun, Freeze, Stone (both phases)

---

## 12. Dispel Interaction

Dispel (Sage skill, ID 289) attempts to remove all stat changes, buffs, and some status effects from a target.

### Removed by Dispel

- All standard buffs (Blessing, Increase AGI, Angelus, etc.)
- Two Hand Quicken, Adrenaline Rush, Weapon Perfection
- Stun, Sleep, Bleeding (some implementations)
- Most positive buffs

### NOT Removed by Dispel (UNDISPELLABLE)

- Enchant Deadly Poison (Assassin Cross)
- Chemical Protection (Alchemist) -- all 4 types
- Strip/Divest effects (Rogue)
- Soul Link buffs (Soul Linker)
- Auto Berserk
- Endure
- Song/Dance performance effects
- Equipment-based status immunities
- Curse, Poison (status ailments -- Dispel removes buffs, not ailments)

### Golden Thief Bug Card

Equipment: Shield card. Effect: Blocks ALL magic including Dispel. Target with GTB cannot be Dispelled at all.

---

## 13. Implementation Checklist

### Core Status Ailments (ro_status_effects.js)

| # | Status | Implemented | Chance Formula | Duration Formula | Periodic Drain | Element Override | Boss Immunity | Damage Break |
|---|--------|------------|---------------|-----------------|---------------|-----------------|--------------|-------------|
| 1 | Stun | YES | YES | YES | N/A | N/A | YES | NO (correct) |
| 2 | Freeze | YES | YES | PARTIAL (missing +srcLUK) | N/A | Water Lv1 YES | YES | YES |
| 3 | Stone (Phase 1 - Petrifying) | YES | YES | YES | N/A | N/A | YES | NO (correct) |
| 4 | Stone (Phase 2 - Petrified) | YES | Fixed 20s | N/A | 1%/5s YES | Earth Lv1 YES | YES | YES |
| 5 | Sleep | YES | YES | YES | N/A | N/A | YES | YES |
| 6 | Poison | YES | YES | YES | 1.5%+2/1s YES | N/A | YES | NO (correct) |
| 7 | Deadly Poison | NO | - | - | - | - | - | - |
| 8 | Blind | YES | YES | PARTIAL (min 15s not enforced) | N/A | N/A | YES | NO (correct) |
| 9 | Silence | YES | YES | YES | N/A | N/A | YES | NO (correct) |
| 10 | Confusion | YES | YES (inverted level) | YES | N/A | N/A | YES | YES |
| 11 | Bleeding | YES | YES | YES | 2%/4s YES | N/A | YES | NO (correct) |
| 12 | Curse | YES | YES (LUK primary) | YES (VIT for duration) | N/A | N/A | YES | NO (correct) |
| 13 | Ankle Snare | YES | Always applies | AGI-based | N/A | N/A | 1/5 duration | NO (correct) |

### Secondary Debuffs (ro_buff_system.js)

| # | Debuff | Implemented | Duration | Stat Effects | Boss Interaction |
|---|--------|------------|----------|-------------|-----------------|
| 1 | Provoke | YES | 30s | ATK+/DEF- | Works |
| 2 | Lex Aeterna | YES | Until hit | 2x damage | Works |
| 3 | Lex Divina | Implicit (via Silence) | Lv-based | Silence | Boss immune |
| 4 | Signum Crucis | YES | Permanent | DEF reduction | Works on Undead/Demon |
| 5 | Decrease AGI | YES | 60s | AGI/Speed reduction | Reduced effect |
| 6 | Quagmire | YES | Lv-based | AGI/DEX/Speed reduction | Works (full effect) |
| 7 | Hallucination | NO | - | - | - |
| 8 | Coma | NO | - | - | - |
| 9 | Strip Weapon | YES | DEX-based | -25% ATK | Works |
| 10 | Strip Shield | YES | DEX-based | -15% DEF | Works |
| 11 | Strip Armor | YES | DEX-based | -40% VIT DEF | Works |
| 12 | Strip Helm | YES | DEX-based | -15% INT/DEX | Works |
| 13 | Ankle Snare | YES | AGI-based | Movement lock | 1/5 duration |
| 14 | Close Confine | YES | 10s | Mutual root | Boss immune |
| 15 | Spider Web | NO | - | - | - |
| 16 | Ensemble Aftermath | YES | 15-30s | Speed penalty | N/A |

### Client-Side Implementation

| Feature | Status | Notes |
|---------|--------|-------|
| Status effect icons in buff bar | YES | BuffBarSubsystem |
| Freeze VFX (ice crystal) | YES | SkillVFXSubsystem Pattern D |
| Stone VFX (grey overlay) | YES | SkillVFXSubsystem Pattern D |
| Stun VFX (stars) | PARTIAL | Needs dedicated effect |
| Sleep VFX (ZZZ) | PARTIAL | Needs dedicated effect |
| Poison VFX (green bubbles) | PARTIAL | Needs dedicated effect |
| Blind screen darkening | NO | Client-side shader needed |
| Confusion input scramble | NO | Client-side input redirect needed |
| status:applied handler | YES | BuffBarSubsystem |
| status:removed handler | YES | BuffBarSubsystem |
| status:tick handler | YES | DamageNumberSubsystem |
| Damage break broadcast | YES | checkDamageBreakStatuses() |

---

## 14. Gap Analysis vs Sabri_MMO

### Missing Status Effects

1. **Deadly Poison (SC_DPOISON)** -- Not implemented. Needed for Assassin Cross EDP mechanic. Faster drain than normal poison, CAN kill (no 25% HP floor).

2. **Hallucination** -- Not implemented. Low priority (primarily visual/cosmetic). Needs client-side screen distortion shader.

3. **Coma** -- Not implemented. Needed for Turtle General Card and select monster skills. Instant HP/SP = 1 effect.

4. **Spider Web** -- Not implemented. Needed for Sage quest skill. Root + Fire vulnerability.

### Formula Gaps

5. **Freeze duration should include +srcLUK** -- Current implementation likely uses standard duration formula. rAthena pre-renewal Freeze uniquely adds `+ 10 * srcLUK` to duration, making high-LUK Wizards more effective.

6. **Blind minimum duration** -- Should be 15,000ms minimum (higher than the standard 1,000ms minimum). Current implementation may use 1,000ms minimum.

7. **Poison 25% HP floor** -- Implemented correctly (minHpPercent: 0.25 in poison drain config). Verified.

8. **Stone Phase 1 NOT breakable by damage** -- Implemented correctly (breakOnDamage: false on petrifying). Verified.

9. **OPT1 mutual exclusion** -- When applying an OPT1 status (Stun/Freeze/Stone/Sleep), any existing OPT1 status should be removed first. Need to verify this is implemented.

### Client-Side Gaps

10. **Blind screen darkening** -- No client-side shader for fog of war / visibility reduction.

11. **Confusion input scramble** -- No client-side logic to randomize movement direction.

12. **Hallucination screen distortion** -- No client-side shader for wavy screen + fake damage numbers.

13. **Status-specific VFX** -- Stun (stars), Sleep (ZZZ), Poison (green bubbles), Bleeding (blood drops), Curse (dark aura) need dedicated Niagara effects.

### Boss Immunity Completeness

14. **Verify boss immunity covers all 11 core statuses** -- Current `modeFlags.statusImmune` should reject all SC types. Need to verify Deadly Poison, Coma.

15. **Quagmire boss bypass** -- Quagmire must work on bosses even when `statusImmune = true`. Verify this is handled (Quagmire is a buff debuff, not a status ailment, so it should bypass).

16. **Strip boss bypass** -- Divest skills must work on bosses. Verify Chemical Protection blocking is checked before strip.

### Missing Interaction Checks

17. **Sleep 100% HIT + 2x crit** -- Attacks against sleeping targets should always hit and have doubled critical rate. Need to verify this is in the combat tick.

18. **Frozen/Stone DEF -50% / MDEF +25%** -- Implemented via statMods in ro_status_effects.js. Verified.

19. **Damage break broadcasts** -- All damage paths must call `checkDamageBreakStatuses()`. Need to verify coverage (auto-attack, skills, traps, ground effects, reflect damage).

20. **10+ level advantage bypass** -- When source is 10+ levels above target, stat resistance may be ignored for Poison. Low priority but historically accurate.

---

*Document generated from 20+ web sources cross-referenced with rAthena pre-renewal source, iRO Wiki Classic, RateMyServer Official Formulas Guide, and existing Sabri_MMO implementation analysis.*
