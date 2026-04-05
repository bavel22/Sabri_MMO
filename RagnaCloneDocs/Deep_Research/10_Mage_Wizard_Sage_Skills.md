# Mage / Wizard / Sage Skills -- Deep Research (Pre-Renewal Classic)

> **Sources:** iRO Wiki Classic, iRO Wiki (Renewal cross-reference), RateMyServer (Pre-Re), rAthena pre-renewal skill_db, Ragnarok Research Lab, divine-pride.net, WarpPortal Forums, rAthena Forums, GameFAQs RO Guides, StrategyWiki, project docs (03_Skills_Complete.md, Mage_Skills_Audit_And_Fix_Plan.md, Wizard_Class_Research.md, Sage_Class_Research.md)
> **Date:** 2026-03-22
> **Scope:** All Mage (IDs 200-213), Wizard (IDs 800-813), and Sage (IDs 1400-1421) skills

---

## Table of Contents

1. [Cast Time & Interruption Mechanics](#1-cast-time--interruption-mechanics)
2. [Mage Skills (IDs 200-213)](#2-mage-skills-ids-200-213)
3. [Wizard Skills (IDs 800-813)](#3-wizard-skills-ids-800-813)
4. [Sage Skills (IDs 1400-1421)](#4-sage-skills-ids-1400-1421)
5. [Ground Effect Skills](#5-ground-effect-skills)
6. [Hindsight / Auto Spell Mechanics](#6-hindsight--auto-spell-mechanics)
7. [Abracadabra Complete Skill List](#7-abracadabra-complete-skill-list)
8. [Free Cast (Move While Casting) Mechanics](#8-free-cast-move-while-casting-mechanics)
9. [Spell Interruption and Phen Card](#9-spell-interruption-and-phen-card)
10. [Skill Trees & Prerequisites](#10-skill-trees--prerequisites)
11. [Implementation Checklist](#11-implementation-checklist)
12. [Gap Analysis vs Current Codebase](#12-gap-analysis-vs-current-codebase)

---

## 1. Cast Time & Interruption Mechanics

### Pre-Renewal Cast Time Formula

```
Final Cast Time = Base Cast Time * (1 - DEX / 150) * (1 - Suffragium%) * (1 - ItemCastReduction%)
```

- At **150 DEX**, variable cast time = **0** (instant cast).
- At 75 DEX, cast time is halved.
- There is **NO fixed cast time** in pre-renewal. All cast time is variable.
- Suffragium (Priest skill): 15% / 30% / 45% reduction.
- Phen Card: +25% cast time, but casting **cannot be interrupted** by damage.
- Equipment reductions apply multiplicatively (not additively).

### After-Cast Delay (ACD)

- Global cooldown after skill finishes. No other skills usable during ACD.
- Reduced by Bragi's Poem (A Poem of Bragi).
- **NOT** reduced by DEX.
- Separate from skill-specific cooldowns.

### Skill Interruption Rules

- Taking damage during casting **interrupts** the cast (cast cancelled, SP NOT consumed).
- **Phen Card**: prevents interruption from damage (but adds +25% cast time).
- **Endure**: prevents flinch but does NOT prevent cast interruption in pre-renewal.
- **Free Cast** (Sage): allows movement during casting but does NOT prevent damage interruption.
- Moving always cancels casting (unless Free Cast is learned).
- Trick: equip Phen Card after cast starts to get anti-interruption without the +25% penalty (cast time calculated at cast start only).

---

## 2. Mage Skills (IDs 200-213)

### 2.1 Cold Bolt (ID 200) -- Water

| Property | Value |
|----------|-------|
| Type | Active, single target, multi-hit |
| Max Level | 10 |
| Element | Water |
| Range | 9 cells |
| Hits | 1 per level (Lv1=1 ... Lv10=10) |
| Damage | 100% MATK per hit |
| SP Cost | `10 + 2*Lv` = 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 |
| Cast Time | `Lv * 0.7s` = 700, 1400, 2100, 2800, 3500, 4200, 4900, 5600, 6300, 7000 ms |
| After-Cast Delay | `0.8 + Lv * 0.2s` = 1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800 ms |
| Prerequisites | None |

All bolts hit simultaneously as a single damage bundle. All three bolt spells (Cold/Fire/Lightning) share identical formulas -- only element differs.

### 2.2 Fire Bolt (ID 201) -- Fire

Identical to Cold Bolt in every respect except element is Fire.

### 2.3 Lightning Bolt (ID 202) -- Wind

Identical to Cold Bolt in every respect except element is Wind.

### 2.4 Napalm Beat (ID 203) -- Ghost

| Property | Value |
|----------|-------|
| Type | Active, single target with 3x3 splash |
| Max Level | 10 |
| Element | Ghost |
| Range | 9 cells |
| Hits | 1 (single hit at all levels) |
| Damage | `(70 + 10*Lv)%` MATK = 80, 90, 100, 110, 120, 130, 140, 150, 160, 170% |
| Splash | 3x3 cells around primary target |
| Damage Split | Total damage divided equally among all targets hit |
| SP Cost | Lv1-3: 9, Lv4-6: 12, Lv7-9: 15, Lv10: 18 |
| Cast Time | 1000ms (fixed, all levels) -- some sources show slight decrease at higher levels |
| After-Cast Delay | 1000, 1000, 1000, 900, 900, 800, 800, 700, 600, 500 ms |
| Prerequisites | None |

### 2.5 Increase SP Recovery (ID 204) -- Passive

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| SP Regen (flat) | `3 * Lv` per tick (+3 to +30 per 8s standing / 4s sitting) |
| MaxSP% Bonus | `0.2% * Lv` of MaxSP added to regen per tick |
| SP Item Potency | `+2% * Lv` to SP recovery items (+2% to +20%) |
| Prerequisites | None |

**Pre-Renewal SP regen formula:**
```
SPR = 1 + floor(MaxSP / 100) + floor(INT / 6)
if (INT >= 120): SPR += floor(INT / 2 - 56)
SPR += IncSPRecoveryFlat
SPR = floor(SPR * (1 + MaxSPPctBonus))
```
Ticks every 8s standing, 4s sitting. Magnificat halves intervals.

### 2.6 Sight (ID 205) -- Fire

| Property | Value |
|----------|-------|
| Type | Active, self buff |
| Max Level | 1 |
| Element | Fire |
| SP Cost | 10 |
| Cast Time | 0 (instant) |
| Duration | 10 seconds |
| AoE | 7x7 cells around caster |
| Effect | Reveals hidden/cloaked enemies |
| Prerequisites | None |

Consumed by Sight Rasher (Wizard skill). Does NOT damage. Does NOT reveal Chase Walk (Stalker).

### 2.7 Stone Curse (ID 206) -- Earth

| Property | Value |
|----------|-------|
| Type | Active, single target debuff |
| Max Level | 10 |
| Element | Earth |
| Range | 2 cells |
| SP Cost | `26 - Lv` = 25, 24, 23, 22, 21, 20, 19, 18, 17, 16 |
| Cast Time | 1000ms (fixed) |
| Stone Chance | `(20 + 4*Lv)%` = 24, 28, 32, 36, 40, 44, 48, 52, 56, 60% |
| Catalyst | 1 Red Gemstone |

**Gem consumption rules:**
- Lv1-5: consumed every cast regardless of success.
- Lv6-10: consumed only on success.

**Two-phase petrification:**
1. **Phase 1 "Petrifying"** (~5s): Target can move and use items but cannot attack or use skills. Being attacked does NOT cancel. Casting Stone Curse on an already-petrifying target NEGATES the effect (cures it).
2. **Phase 2 "Stone"** (20s): Full immobilize, element changes to Earth Lv1, DEF reduced by 50%, MDEF increased by 25%. HP drains 1% every 5s until 25% HP remains. Physical or magical damage breaks the stone.

**Resistance formula (rAthena pre-renewal):**
```
adjustedChance = baseChance - baseChance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK
```

**Immunities:** Boss-protocol and Undead-element monsters immune.

### 2.8 Fire Ball (ID 207) -- Fire

| Property | Value |
|----------|-------|
| Type | Active, single target with 5x5 splash |
| Max Level | 10 |
| Element | Fire |
| Range | 9 cells |
| Hits | 1 |
| Damage | `(70 + 10*Lv)%` MATK = 80-170% |
| Splash | 5x5 cells around target (full damage to ALL, not split) |
| SP Cost | 25 (fixed, all levels) |
| Cast Time | Lv1-5: 1500ms, Lv6-10: 1000ms |
| After-Cast Delay | Lv1-5: 1500ms, Lv6-10: 1000ms |
| Prerequisites | Fire Bolt Lv4 |

### 2.9 Frost Diver (ID 208) -- Water

| Property | Value |
|----------|-------|
| Type | Active, single target + freeze |
| Max Level | 10 |
| Element | Water |
| Range | 9 cells |
| Hits | 1 |
| Damage | `(100 + 10*Lv)%` MATK = 110-200% |
| SP Cost | `26 - Lv` = 25, 24, ..., 16 |
| Cast Time | 800ms (fixed) |
| After-Cast Delay | 1500ms (fixed) |
| Freeze Chance | `(35 + 3*Lv)%` = 38, 41, 44, 47, 50, 53, 56, 59, 62, 65% |
| Freeze Duration | `3 * Lv` seconds = 3, 6, 9, ..., 30s |
| Prerequisites | Cold Bolt Lv5 |

**Frozen status properties:**
- Target becomes Water Lv1 element
- DEF reduced by 50%, MDEF increased by 25%
- Full immobilize
- Broken by physical/magical damage (except Ice element)

**Resistance (rAthena pre-renewal):**
```
adjustedChance = baseChance - baseChance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK
adjustedDuration = baseDuration - baseDuration * targetHardMDEF / 100 - 10 * srcLUK
```

**Immunities:** Boss-protocol and Undead-element monsters.

### 2.10 Fire Wall (ID 209) -- Fire / Ground

| Property | Value |
|----------|-------|
| Type | Active, ground placement |
| Max Level | 10 |
| Element | Fire |
| Range | 9 cells |
| Placement | 1x3 cells perpendicular to caster-target line |
| Damage | 50% MATK per hit |
| Hit Count | `Lv + 2` = 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 hits per cell |
| Duration | `Lv + 4` seconds = 5, 6, 7, 8, 9, 10, 11, 12, 13, 14s |
| SP Cost | 40 (fixed) |
| Cast Time | `2.15 - 0.15*Lv` sec = 2000, 1850, 1700, 1550, 1400, 1250, 1100, 950, 800, 650 ms |
| After-Cast Delay | 0 |
| Max Concurrent | 3 walls per caster |
| Knockback | 2 cells (on contact) |
| Hit Rate | ~40-50 hits per second (rapid fire on contact) |
| Prerequisites | Fire Ball Lv5, Sight Lv1 |

**Special rules:**
- Boss-flagged monsters are NOT knocked back but must absorb all hits before proceeding.
- Undead-property monsters are NEITHER blocked NOR knocked back -- they pass through freely.
- Caster incapacitation (Stun, Frozen, Stone) prevents the wall from processing attacks.
- Diagonal walls spawn additional segments to prevent "holes."

### 2.11 Soul Strike (ID 210) -- Ghost

| Property | Value |
|----------|-------|
| Type | Active, single target, multi-hit |
| Max Level | 10 |
| Element | Ghost |
| Range | 9 cells |
| Hits | `floor((Lv + 1) / 2)` = 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 |
| Damage | 100% MATK per hit |
| Undead Bonus | `+5% * Lv` = +5% to +50% vs Undead element |
| SP Cost | 18, 14, 24, 20, 30, 26, 36, 32, 42, 38 (zigzag pattern) |
| Cast Time | 500ms (fixed, all levels) |
| After-Cast Delay | 1200, 1000, 1400, 1200, 1600, 1400, 1800, 1600, 2000, 1800 ms (zigzag) |
| Prerequisites | Napalm Beat Lv4 |

### 2.12 Safety Wall (ID 211) -- Neutral / Ground

| Property | Value |
|----------|-------|
| Type | Active, ground placement |
| Max Level | 10 |
| Range | 9 cells |
| Area | 1x1 cell |
| Hits Blocked | `Lv + 1` = 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 |
| Duration | `5 * Lv` sec = 5, 10, 15, ..., 50s |
| Blocks | Melee physical attacks ONLY (not ranged, not magic) |
| SP Cost | Lv1-3: 30, Lv4-6: 35, Lv7-10: 40 |
| Cast Time | 4000, 3500, 3000, 2500, 2000, 1500, 1000, 1000, 1000, 1000 ms |
| After-Cast Delay | 0 |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Napalm Beat Lv7, Soul Strike Lv5 |

### 2.13 Thunderstorm (ID 212) -- Wind / Ground AoE

| Property | Value |
|----------|-------|
| Type | Active, ground AoE |
| Max Level | 10 |
| Element | Wind |
| Range | 9 cells |
| AoE | 5x5 cells |
| Hits | 1 per level (Lv1=1 ... Lv10=10) |
| Damage | 80% MATK per hit |
| SP Cost | `24 + 5*Lv` = 29, 34, 39, 44, 49, 54, 59, 64, 69, 74 |
| Cast Time | `Lv * 1s` = 1000, 2000, ..., 10000 ms |
| After-Cast Delay | 2000ms (fixed) |
| Prerequisites | Lightning Bolt Lv4 |

### 2.14 Energy Coat (ID 213) -- Quest Skill

| Property | Value |
|----------|-------|
| Type | Active, self buff |
| Max Level | 1 |
| SP Cost | 30 |
| Cast Time | 0 |
| Duration | 300 seconds (5 min) |

| Current SP % | Damage Reduction | SP Drain per Hit |
|--------------|-----------------|-----------------|
| >80% | 30% | 3% of current SP |
| 61-80% | 24% | 2.5% |
| 41-60% | 18% | 2% |
| 21-40% | 12% | 1.5% |
| 1-20% | 6% | 1% |

Reduces all incoming physical damage. SP is drained per hit received.

---

## 3. Wizard Skills (IDs 800-813)

### 3.1 Jupitel Thunder (ID 800) -- Wind

| Property | Value |
|----------|-------|
| Type | Active, single target, multi-hit + knockback |
| Max Level | 10 |
| Element | Wind |
| Range | 9 cells |
| Prerequisites | Napalm Beat Lv1, Lightning Bolt Lv1 |

| Lv | SP | Cast (ms) | Hits | KB (cells) | ACD |
|----|----|-----------|------|------------|-----|
| 1 | 20 | 2500 | 3 | 2 | 0 |
| 2 | 23 | 3000 | 4 | 3 | 0 |
| 3 | 26 | 3500 | 5 | 3 | 0 |
| 4 | 29 | 4000 | 6 | 4 | 0 |
| 5 | 32 | 4500 | 7 | 4 | 0 |
| 6 | 35 | 5000 | 8 | 5 | 0 |
| 7 | 38 | 5500 | 9 | 5 | 0 |
| 8 | 41 | 6000 | 10 | 6 | 0 |
| 9 | 44 | 6500 | 11 | 6 | 0 |
| 10 | 47 | 7000 | 12 | 7 | 0 |

- Damage: 100% MATK per hit. Total = hits x MATK.
- Knockback: `floor((hits + 1) / 2)` cells. Disabled in WoE. Boss-protocol immune to KB (damage still applies).
- SP formula: `17 + Lv * 3`
- Cast time formula: `2000 + Lv * 500` ms

### 3.2 Lord of Vermilion (ID 801) -- Wind / Ground AoE

| Property | Value |
|----------|-------|
| Type | Active, ground AoE |
| Max Level | 10 |
| Element | Wind |
| Range | 9 cells |
| AoE | 9x9 cells (effective 11x11 due to splash) |
| Prerequisites | Thunderstorm Lv1, Jupitel Thunder Lv5 |

| Lv | SP | Cast (ms) | MATK% per wave | Total MATK% (4 waves) | Blind % |
|----|----|-----------|----------------|----------------------|---------|
| 1 | 60 | 15000 | 100 | 400 | 4 |
| 2 | 64 | 14500 | 120 | 480 | 8 |
| 3 | 68 | 14000 | 140 | 560 | 12 |
| 4 | 72 | 13500 | 160 | 640 | 16 |
| 5 | 76 | 13000 | 180 | 720 | 20 |
| 6 | 80 | 12500 | 200 | 800 | 24 |
| 7 | 84 | 12000 | 220 | 880 | 28 |
| 8 | 88 | 11500 | 240 | 960 | 32 |
| 9 | 92 | 11000 | 260 | 1040 | 36 |
| 10 | 96 | 10500 | 280 | 1120 | 40 |

- SP formula: `56 + 4 * Lv`
- Cast time formula: `15500 - 500 * Lv` ms
- After-Cast Delay: 5000ms fixed
- Damage delivered as a bundle of 20 visual hits over 4 waves (~1 wave per second)
- Blind chance: `4 * Lv` % per wave (boss immune to blind)
- Does NOT stack: two LoV on same location have no additional effect
- Immunity: targets hit are immune for 0.5s (prevents double-dipping)
- Requires line of sight to target

### 3.3 Meteor Storm (ID 802) -- Fire / Ground AoE

| Property | Value |
|----------|-------|
| Type | Active, ground AoE (random meteor rain) |
| Max Level | 10 |
| Element | Fire |
| Range | 9 cells |
| Target Area | 7x7 cells (meteors fall randomly within) |
| Splash per Meteor | 7x7 cells each (potential 13x13 effective area) |
| Prerequisites | Thunderstorm Lv1, Sight Rasher Lv2 |

| Lv | SP | Cast (ms) | Meteors | Hits/Meteor | Stun % | Total Hits |
|----|----|-----------|---------|-------------|--------|------------|
| 1 | 20 | 15000 | 2 | 1 | 3 | 2 |
| 2 | 24 | 15000 | 2 | 1 | 6 | 2 |
| 3 | 30 | 15000 | 3 | 2 | 9 | 6 |
| 4 | 34 | 15000 | 3 | 2 | 12 | 6 |
| 5 | 40 | 15000 | 4 | 3 | 15 | 12 |
| 6 | 44 | 15000 | 4 | 3 | 18 | 12 |
| 7 | 50 | 15000 | 5 | 4 | 21 | 20 |
| 8 | 54 | 15000 | 5 | 4 | 24 | 20 |
| 9 | 60 | 15000 | 6 | 5 | 27 | 30 |
| 10 | 64 | 15000 | 6 | 5 | 30 | 30 |

- Damage: 125% MATK per hit (each hit independently rolls MATK)
- SP cost array: [20, 24, 30, 34, 40, 44, 50, 54, 60, 64] (non-linear +4/+6 zigzag)
- Cast time: 15000ms fixed (all variable, reduced by DEX)
- After-Cast Delay: `2000 + 500 * Lv` ms = 2500, 3000, ..., 7000
- Meteor count: `floor((Lv + 2) / 2)` = 2, 2, 3, 3, 4, 4, 5, 5, 6, 6
- Hits per meteor: `floor((Lv + 1) / 2)` = 1, 1, 2, 2, 3, 3, 4, 4, 5, 5
- Stun chance: `3 * Lv` % per hit (boss immune)
- Meteors fall at random positions within the target area over ~2-3 seconds
- A single target CAN be hit by multiple meteors if splashes overlap
- **Stacks:** multiple Meteor Storms can overlap and all deal full damage (unlike LoV/SG)
- Max theoretical damage at Lv10 with full overlap: 30 x 125% MATK = 3750% MATK

### 3.4 Storm Gust (ID 803) -- Water / Ground AoE

| Property | Value |
|----------|-------|
| Type | Active, ground AoE (blizzard) |
| Max Level | 10 |
| Element | Water |
| Range | 9 cells |
| AoE | 7x7 base, each cell splashes 3x3 = effective 9x9 to 11x11 |
| Prerequisites | Frost Diver Lv1, Jupitel Thunder Lv3 |

| Lv | SP | Cast (ms) | MATK% per hit | Total MATK% (10 hits) |
|----|----|-----------|---------------|----------------------|
| 1 | 78 | 6000 | 140 | 1400 |
| 2 | 78 | 7000 | 180 | 1800 |
| 3 | 78 | 8000 | 220 | 2200 |
| 4 | 78 | 9000 | 260 | 2600 |
| 5 | 78 | 10000 | 300 | 3000 |
| 6 | 78 | 11000 | 340 | 3400 |
| 7 | 78 | 12000 | 380 | 3800 |
| 8 | 78 | 13000 | 420 | 4200 |
| 9 | 78 | 14000 | 460 | 4600 |
| 10 | 78 | 15000 | 500 | 5000 |

- Damage formula: `(100 + 40 * Lv)%` MATK per hit, 10 hits total
- SP cost: 78 fixed
- Cast time: `5000 + 1000 * Lv` ms
- After-Cast Delay: 5000ms fixed
- Duration: ~4.6 seconds (10 hits over ~460ms intervals)

**Freeze Mechanic (CRITICAL -- Pre-Renewal):**
- The **3rd hit** of Storm Gust on the same target has a **150% base chance** to freeze
- This is NOT "every 3 hits freeze" -- the 3rd hit has the check, then the counter resets
- High MDEF can reduce the 150% below 100% for a chance to resist
- Boss-protocol and Undead-element monsters are immune to freeze
- The hit counter is GLOBAL -- hits from different Storm Gust instances count together
- Once frozen, subsequent hits break the freeze (unfreezing the target)

**Push mechanic:** Each hit pushes enemies in a random direction.

**Non-stacking:** Two Storm Gusts on the same location have no more effect than one. Targets are immune for 0.5s after being hit.

### 3.5 Earth Spike (ID 804) -- Earth

| Property | Value |
|----------|-------|
| Type | Active, single target, multi-hit |
| Max Level | 5 |
| Element | Earth |
| Range | 9 cells |
| Hits | = Lv (1-5) |
| Damage | 100% MATK per hit (pre-renewal) |
| SP Cost | `10 + 2*Lv` = 12, 14, 16, 18, 20 |
| Cast Time | `Lv * 700` ms = 700, 1400, 2100, 2800, 3500 |
| After-Cast Delay | `800 + Lv * 200` ms = 1000, 1200, 1400, 1600, 1800 |
| Prerequisites | Stone Curse Lv1 |

Can hit Hidden enemies. Functionally identical to bolt spells but Earth element and capped at Lv5.

**Note on MATK%:** Some sources (including the project Wizard_Class_Research.md) list this as 200% MATK per hit. The rAthena pre-renewal data uses 100% per hit. The Sage version (ID 1417) documentation also shows some sources claiming 200%. For consistency with rAthena pre-renewal, use 100% per hit.

### 3.6 Heaven's Drive (ID 805) -- Earth / Ground AoE

| Property | Value |
|----------|-------|
| Type | Active, ground AoE |
| Max Level | 5 |
| Element | Earth |
| Range | 9 cells |
| AoE | 5x5 cells |
| Hits | = Lv (1-5) |
| Damage | 125% MATK per hit (pre-renewal verified) |
| SP Cost | `24 + 4*Lv` = 28, 32, 36, 40, 44 |
| Cast Time | `Lv * 1000` ms = 1000, 2000, 3000, 4000, 5000 |
| After-Cast Delay | 500ms flat |
| Prerequisites | Earth Spike Lv3 |

Can hit Hidden enemies. All targets in 5x5 take full damage (not split).

### 3.7 Quagmire (ID 806) -- Earth / Ground Debuff

| Property | Value |
|----------|-------|
| Type | Active, ground debuff zone |
| Max Level | 5 |
| Element | Earth |
| Range | 9 cells |
| AoE | 5x5 cells |
| Cast Time | 0 (instant) |
| Cast Delay | 1 second |
| Max Concurrent | 3 per caster |
| Prerequisites | Heaven's Drive Lv1 |

| Lv | SP | AGI/DEX Reduction | Move Speed Red. | Duration |
|----|----|-------------------|-----------------|----------|
| 1 | 5 | 10 | 50% | 5s |
| 2 | 10 | 20 | 50% | 10s |
| 3 | 15 | 30 | 50% | 15s |
| 4 | 20 | 40 | 50% | 20s |
| 5 | 25 | 50 | 50% | 25s |

**Reduction formula:** `10 * Lv` AGI and DEX (10-50 per iRO Wiki Classic).
**Caps:** Monster stats reduced by max 50% of base. Player stats reduced by max 25% of base.
**Movement speed:** Flat 50% reduction regardless of level.
**Buff stripping:** Removes Increase AGI, Two-Hand Quicken, Spear Quicken, Adrenaline Rush, Wind Walker, One-Hand Quicken when targets enter the zone.
**Debuff lingers:** Persists for ~5 seconds after leaving the area.
**Boss immunity:** Boss-protocol monsters are immune (varies by server -- rAthena default is bosses ARE affected but with capped reduction).

### 3.8 Water Ball (ID 807) -- Water

| Property | Value |
|----------|-------|
| Type | Active, single target, multi-hit |
| Max Level | 5 |
| Element | Water |
| Range | 9 cells |
| Requirements | Must be on/near water cells |
| Prerequisites | Cold Bolt Lv1, Lightning Bolt Lv1 |

| Lv | SP | Cast (ms) | MATK% / hit | Max Hits | Water Grid |
|----|----|-----------|-------------|----------|------------|
| 1 | 15 | 1000 | 130 | 1 | 1x1 |
| 2 | 20 | 2000 | 160 | 4 | 3x3 |
| 3 | 20 | 3000 | 190 | 9 | 3x3 |
| 4 | 25 | 4000 | 220 | 9 | 5x5 |
| 5 | 25 | 5000 | 250 | 25 | 5x5 |

- Damage: `(100 + 30*Lv)%` MATK per hit
- SP cost: [15, 20, 20, 25, 25]
- Cast time: `1000 * Lv` ms
- Actual hits = min(maxHits, availableWaterCells)
- Water cells consumed during use
- Deluge (Sage) and Ice Wall (destroyed cells) create usable water cells
- Lv5 on full water = 25 x 250% MATK = 6250% MATK (highest single-target DPS in game)

### 3.9 Ice Wall (ID 808) -- Water / Ground Obstacle

| Property | Value |
|----------|-------|
| Type | Active, ground obstacle |
| Max Level | 10 |
| Element | Water Lv1 |
| Range | 9 cells |
| Wall | 5 cells long perpendicular to caster facing |
| SP Cost | 20 (fixed) |
| Cast Time | 0 (instant) |
| Cast Delay | ASPD-based |
| Prerequisites | Stone Curse Lv1, Frost Diver Lv1 |

| Lv | HP per Cell | Duration Calc (durability/50 sec) |
|----|------------|-----------------------------------|
| 1 | 400 | 8s |
| 2 | 600 | 12s |
| 3 | 800 | 16s |
| 4 | 1000 | 20s |
| 5 | 1200 | 24s |
| 6 | 1400 | 28s |
| 7 | 1600 | 32s |
| 8 | 1800 | 36s |
| 9 | 2000 | 40s |
| 10 | 2200 | 44s |

- HP formula: `200 + 200 * Lv` per cell
- Durability decays by 50 per second naturally
- Each cell is individually destructible with its own HP
- Blocks movement for monsters AND players
- Blocks ranged physical attacks (arrows)
- Does NOT block magic
- Fire element skills deal extra damage (Water Lv1 element)
- Destroyed cells become "water cells" (enables Water Ball)
- Disabled in WoE; functional in PvP

### 3.10 Sight Rasher (ID 809) -- Fire / Self AoE

| Property | Value |
|----------|-------|
| Type | Active, self-centered AoE |
| Max Level | 10 |
| Element | Fire |
| AoE | 7x7 cells around caster |
| Cast Time | 500ms |
| After-Cast Delay | 2000ms |
| Requirement | Sight buff must be active (consumed on use) |
| Prerequisites | Sight Lv1 |

| Lv | SP | MATK% | KB (cells) |
|----|----|-------|------------|
| 1 | 35 | 120 | 5 |
| 2 | 37 | 140 | 5 |
| 3 | 39 | 160 | 5 |
| 4 | 41 | 180 | 5 |
| 5 | 43 | 200 | 5 |
| 6 | 45 | 220 | 5 |
| 7 | 47 | 240 | 5 |
| 8 | 49 | 260 | 5 |
| 9 | 51 | 280 | 5 |
| 10 | 53 | 300 | 5 |

- Damage: `(100 + 20*Lv)%` MATK = 120-300%
- SP: `33 + 2*Lv`
- Knockback: 5 cells outward from caster (disabled in WoE)
- Fires 8 visual fireballs but damage is single AoE check

### 3.11 Fire Pillar (ID 810) -- Fire / Ground Trap

| Property | Value |
|----------|-------|
| Type | Active, ground trap |
| Max Level | 10 |
| Element | Fire |
| Range | 9 cells |
| Area | 1x1 cell (trap point) |
| SP Cost | 75 (fixed) |
| Duration | 30 seconds |
| Max Concurrent | 5 per caster |
| Catalyst | Blue Gemstone (Lv6-10 only) |
| Prerequisites | Fire Wall Lv1 |

| Lv | Cast (ms) | Hits | Damage Formula |
|----|-----------|------|----------------|
| 1 | 3000 | 3 | 3 x (50 + MATK/5) |
| 2 | 2700 | 4 | 4 x (50 + MATK/5) |
| 3 | 2400 | 5 | 5 x (50 + MATK/5) |
| 4 | 2100 | 6 | 6 x (50 + MATK/5) |
| 5 | 1800 | 7 | 7 x (50 + MATK/5) |
| 6 | 1500 | 8 | 8 x (50 + MATK/5) |
| 7 | 1200 | 9 | 9 x (50 + MATK/5) |
| 8 | 900 | 10 | 10 x (50 + MATK/5) |
| 9 | 600 | 11 | 11 x (50 + MATK/5) |
| 10 | 300 | 12 | 12 x (50 + MATK/5) |

- Cast time: `3300 - 300*Lv` ms
- Hits: `Lv + 2`
- **IGNORES MDEF entirely** -- damage is `(50 + MATK/5) * hits`
- Trap activates when enemy walks onto it -- all hits delivered at once
- Cannot be placed on occupied cells
- Invisible to enemies in PvP

### 3.12 Frost Nova (ID 811) -- Water / Self AoE Freeze

| Property | Value |
|----------|-------|
| Type | Active, self-centered AoE freeze |
| Max Level | 10 |
| Element | Water |
| AoE | 5x5 cells (some sources say 7x7) |
| Prerequisites | Frost Diver Lv1, Ice Wall Lv1 |

| Lv | SP | Cast (ms) | MATK% | Freeze % |
|----|----|-----------|-------|----------|
| 1 | 45 | 6000 | 73 | 38 |
| 2 | 43 | 5600 | 80 | 43 |
| 3 | 41 | 5200 | 87 | 48 |
| 4 | 39 | 4800 | 94 | 53 |
| 5 | 37 | 4400 | 101 | 58 |
| 6 | 35 | 4000 | 108 | 63 |
| 7 | 33 | 4000 | 115 | 68 |
| 8 | 31 | 4000 | 122 | 73 |
| 9 | 29 | 4000 | 129 | 78 |
| 10 | 27 | 4000 | 136 | 83 |

- Damage: `(66 + 7*Lv)%` MATK
- SP: `47 - 2*Lv`
- Cast time: Lv1-6: `6600 - 400*Lv`; Lv7-10: 4000ms fixed
- Freeze chance: `33 + 5*Lv` %
- Freeze duration: `1500 * Lv` ms (per rAthena pre-re session fixes)
- Boss/Undead immune to freeze

### 3.13 Sense / Monster Property (ID 812) -- Neutral

| Property | Value |
|----------|-------|
| Type | Active, single target (information) |
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 0 (instant) |
| Range | 9 cells |
| Prerequisites | None |

Reveals target monster stats: HP/MaxHP, element + level, race, size, base level, DEF, MDEF, base EXP, job EXP. Party members also see the result. Works only on monsters.

### 3.14 Sight Blaster (ID 813) -- Fire / Quest Skill

| Property | Value |
|----------|-------|
| Type | Active, self buff (reactive damage) |
| Max Level | 1 |
| Element | Fire |
| SP Cost | 40 |
| Cast Time | 700ms (some sources: instant) |
| Duration | 120 seconds |
| Damage | 100% MATK Fire |
| Knockback | 3 cells |
| Trigger Range | ~2 cells (melee proximity) |
| Quest Req. | Job Level 40 as Wizard |

Protective fireball orbiting caster. When an enemy enters within 2 cells, automatically attacks that enemy with 100% MATK fire damage and 3-cell knockback. Single use -- skill ends after activation. If no trigger in 120s, expires.

---

## 4. Sage Skills (IDs 1400-1421)

### 4.1 Advanced Book (ID 1400) -- Passive

| Lv | ATK Bonus (Books) | ASPD Bonus |
|----|--------------------|-----------|
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

### 4.2 Cast Cancel (ID 1401) -- Active

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 2 |
| Cast Time | 0 (instant) |
| Target | Self |
| Prerequisite | Advanced Book Lv2 |

| Lv | SP Retained on Cancel |
|----|-----------------------|
| 1 | 10% |
| 2 | 30% |
| 3 | 50% |
| 4 | 70% |
| 5 | 90% |

Cancels own ongoing cast. In RO Classic, SP is pre-consumed at cast start, so this refunds a percentage. In our system (SP consumed at execution), it simply cancels without needing a refund.

### 4.3 Hindsight / Auto Spell (ID 1402) -- Active

See [Section 6: Hindsight / Auto Spell Mechanics](#6-hindsight--auto-spell-mechanics) for full details.

### 4.4 Dispell (ID 1403) -- Active

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 1 (some sources say 0; rAthena uses 1) |
| Cast Time | 2000ms (0.4s fixed + 1.6s variable in renewal; pre-renewal all variable) |
| After-Cast Delay | 2000ms (ASPD-based per some sources) |
| Target | Single target (enemy or player) |
| Range | 9 cells |
| Catalyst | 1 Yellow Gemstone |
| Prerequisite | Spell Breaker Lv3 |

| Lv | Success Rate |
|----|-------------|
| 1 | 60% |
| 2 | 70% |
| 3 | 80% |
| 4 | 90% |
| 5 | 100% |

**Undispellable buffs (cannot be removed):**
- Hindsight (Auto Spell)
- Wedding / Riding / Cart status
- Chemical Protection skills (Alchemist)
- Enchant Deadly Poison
- Rogue Divest status effects
- Soul Linker Spirit buffs
- NPC Power Up
- Gunslinger self-buffs
- Item-based movement speed boosts

**Can be removed:** All standard buffs including Endow skills, Blessing, Increase AGI, Angelus, Improve Concentration, Endure, Two-Hand Quicken, Spear Quicken, Provoke, Energy Coat, Volcano/Deluge/Violent Gale stat bonuses.

If skill is not mastered (Lv1-4), target's magic defense lowers the success chance.

### 4.5 Magic Rod (ID 1404) -- Active

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 2 |
| Cast Time | 0 (instant activation) |
| Cast Delay | 1 second |
| Target | Self |
| Prerequisite | Advanced Book Lv4 |

| Lv | Active Duration | SP Absorbed |
|----|----------------|-------------|
| 1 | 400ms | 20% of spell's SP cost |
| 2 | 600ms | 40% |
| 3 | 800ms | 60% |
| 4 | 1000ms | 80% |
| 5 | 1200ms | 100% |

**Absorption rules:**
- Only absorbs **single-target magic** (bolts, Soul Strike, Frost Diver, Jupitel Thunder)
- Does NOT absorb ground AoE magic (Storm Gust, LoV, Meteor Storm, Thunderstorm)
- CAN absorb **multiple spells** during its duration (each restores SP)
- Water Ball projectiles can each be absorbed individually
- If Spell Breaker is used against Magic Rod user, Magic Rod absorbs it and gains 20% of enemy's Max SP
- Energy Coat still drains SP during absorption

### 4.6 Free Cast (ID 1405) -- Passive

See [Section 8: Free Cast Mechanics](#8-free-cast-move-while-casting-mechanics) for full details.

### 4.7 Spell Breaker (ID 1406) -- Active

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 10 |
| Cast Time | 700ms (pre-renewal) |
| Target | Single enemy (must be currently casting) |
| Range | 9 cells |
| Prerequisite | Magic Rod Lv1 |

| Lv | SP Absorbed | Lv5 Special |
|----|-------------|-------------|
| 1 | 0% | -- |
| 2 | 25% of target spell's SP cost | -- |
| 3 | 50% | -- |
| 4 | 75% | -- |
| 5 | 100% | +2% target MaxHP damage, +1% HP drained to caster |

- Interrupts target's ongoing cast, removes from activeCasts
- Boss/Guardian: only 10% success rate for interruption
- Lv5 HP damage/drain applies even if interruption fails on boss
- Ignores Phen Card protection -- always interrupts if success roll passes

### 4.8 Dragonology (ID 1407) -- Passive

| Lv | INT Bonus | Dragon ATK | Dragon Resist |
|----|-----------|-----------|---------------|
| 1 | +1 | +4% | +4% |
| 2 | +1 | +8% | +8% |
| 3 | +1 | +12% | +12% |
| 4 | +2 | +16% | +16% |
| 5 | +3 | +20% | +20% |

Prerequisite: Advanced Book Lv9. Affects Dragon race enemies.

### 4.9 Endow Blaze (ID 1408) -- Fire

### 4.10 Endow Tsunami (ID 1409) -- Water

### 4.11 Endow Tornado (ID 1410) -- Wind

### 4.12 Endow Quake (ID 1411) -- Earth

All four endow skills share identical mechanics:

| Property | Value |
|----------|-------|
| SP Cost | 40 (all levels) |
| Cast Time | 3000ms (pre-renewal, all levels) |
| Target | Single ally (party/guild, or self) |
| Range | 9 cells |
| Prerequisite | Advanced Book Lv5 + respective bolt/Stone Curse Lv1 |

| Lv | Success Rate | Duration |
|----|-------------|----------|
| 1 | 70% | 20 min |
| 2 | 80% | 22.5 min |
| 3 | 90% | 25 min |
| 4 | 100% | 27.5 min |
| 5 | 100% | 30 min |

**Catalysts:**
- Endow Blaze: 1 Red Blood (990)
- Endow Tsunami: 1 Crystal Blue (991)
- Endow Tornado: 1 Wind of Verdure (992)
- Endow Quake: 1 Green Live (993)

**Failure penalty (pre-renewal):** On failure, the target's weapon is BROKEN (ATK = 0 until repaired). Implementation recommendation: skip weapon-breaking, just consume catalyst on failure.

**Mutual exclusion:** Only one weapon endow active at a time. New endow replaces old.
**Dispell:** Endow buffs ARE removed by Dispell. On removal, revert to base weapon element.

### 4.13 Volcano (ID 1412) -- Fire / Ground

| Lv | SP | Fire Dmg Boost | ATK/MATK Bonus | Duration |
|----|----|----------------|----------------|----------|
| 1 | 48 | +10% | +10 | 1 min |
| 2 | 46 | +14% | +15 | 2 min |
| 3 | 44 | +17% | +20 | 3 min |
| 4 | 42 | +19% | +25 | 4 min |
| 5 | 40 | +20% | +30 | 5 min |

Cast time: 5000ms. AoE: 7x7. Range: 2 cells. Catalyst: 1 Blue Gemstone (recasting active buff = no gem cost). Prevents Ice Wall on area.

### 4.14 Deluge (ID 1413) -- Water / Ground

| Lv | SP | Water Dmg Boost | MaxHP Bonus | Duration |
|----|----|-----------------|-------------|----------|
| 1 | 48 | +10% | +5% | 1 min |
| 2 | 46 | +14% | +9% | 2 min |
| 3 | 44 | +17% | +12% | 3 min |
| 4 | 42 | +19% | +14% | 4 min |
| 5 | 40 | +20% | +15% | 5 min |

Acts as shallow water for Water Ball and Aqua Benedicta. Water Ball consumes Deluge cells rapidly. Doubles Blinding Mist duration.

### 4.15 Violent Gale / Whirlwind (ID 1414) -- Wind / Ground

| Lv | SP | Wind Dmg Boost | FLEE Bonus | Duration |
|----|----|----------------|-----------|----------|
| 1 | 48 | +10% | +3 | 1 min |
| 2 | 46 | +14% | +6 | 2 min |
| 3 | 44 | +17% | +9 | 3 min |
| 4 | 42 | +19% | +12 | 4 min |
| 5 | 40 | +20% | +15 | 5 min |

Extends Fire Wall duration by +50%.

**Shared ground effect properties (Volcano/Deluge/Violent Gale):**
- Cannot overlap with other ground effect skills
- Cancelled by Land Protector
- Stat bonuses are element-restricted (Volcano fire boost only helps fire attacks, etc.)
- SP costs decrease with level: 48, 46, 44, 42, 40

### 4.16 Land Protector / Magnetic Earth (ID 1415) -- Neutral / Ground

| Lv | SP | AoE | Duration |
|----|-----|------|----------|
| 1 | 66 | 7x7 | 120s (2 min) |
| 2 | 62 | 7x7 | 165s (2.75 min) |
| 3 | 58 | 9x9 | 210s (3.5 min) |
| 4 | 54 | 9x9 | 255s (4.25 min) |
| 5 | 50 | 11x11 | 300s (5 min) |

- SP: `70 - 4*Lv`
- Duration: `75 + 45*Lv` seconds
- Cast time: 5000ms
- Range: 3 cells
- Catalyst: 1 Blue Gemstone + 1 Yellow Gemstone
- Prerequisite: Volcano Lv3, Deluge Lv3, Violent Gale Lv3

**Nullifies and removes:**
Safety Wall, Pneuma, Warp Portal, Sanctuary, Magnus Exorcismus, Volcano, Deluge, Violent Gale, Fire Wall, Fire Pillar, Thunderstorm, Storm Gust, Lord of Vermilion, Meteor Storm, Quagmire, Ice Wall

**Does NOT block:**
Hunter traps, Grand Cross, non-ground-targeted spells (bolts, single-target magic)

On creation: removes all existing ground effects in area. While active: prevents new ground effects.

### 4.17 Abracadabra / Hocus Pocus (ID 1416) -- Active

See [Section 7: Abracadabra Complete Skill List](#7-abracadabra-complete-skill-list).

### 4.18 Earth Spike (Sage) (ID 1417) -- Earth

Same mechanics as Wizard Earth Spike (ID 804). Different prerequisites: Stone Curse Lv1 + Endow Quake Lv1.

### 4.19 Heaven's Drive (Sage) (ID 1418) -- Earth / Ground AoE

Same mechanics as Wizard Heaven's Drive (ID 805). Different prerequisites: Earth Spike (Sage) Lv1 (vs Wizard requiring Lv3).

### 4.20 Sense (Sage) (ID 1419) -- Neutral

Same as Wizard Sense (ID 812). Same mechanics.

### 4.21 Create Elemental Converter (ID 1420) -- Quest Skill

| Property | Value |
|----------|-------|
| SP Cost | 30 |
| Cast Time | 0 |
| Target | Self (crafting) |

| Converter | Materials |
|-----------|-----------|
| Fire Converter | 3 Scorpion Tails + 1 Blank Scroll |
| Water Converter | 3 Snail's Shells + 1 Blank Scroll |
| Earth Converter | 3 Horns + 1 Blank Scroll |
| Wind Converter | 3 Rainbow Shells + 1 Blank Scroll |

Converters endow own weapon with element for 5 minutes. Usable by any class once crafted.

### 4.22 Elemental Change (ID 1421) -- Quest Skill

| Property | Value |
|----------|-------|
| SP Cost | 30 |
| Cast Time | 2000ms |
| After-Cast Delay | 1000ms |
| Target | Single enemy |
| Range | 9 cells |
| Catalyst | 1 matching Elemental Converter |
| Requirement | Job Level 40 |

Permanently changes target monster's element to the converter's element at Level 1.

---

## 5. Ground Effect Skills

### 5.1 Classification

| Skill | Type | Duration | Damage? | Persistent? |
|-------|------|----------|---------|-------------|
| Fire Wall (209) | Barrier / Damage | Lv+4 sec | Yes (50% MATK/hit) | Yes |
| Safety Wall (211) | Barrier / Defense | 5*Lv sec | No | Yes |
| Storm Gust (803) | Timed AoE Damage | ~4.6s | Yes (10 waves) | Yes |
| Lord of Vermilion (801) | Timed AoE Damage | ~4s | Yes (4 waves) | Yes |
| Meteor Storm (802) | Random AoE Damage | ~3s | Yes (random meteors) | Yes |
| Ice Wall (808) | Obstacle | durability/50 sec | No (breakable) | Yes |
| Fire Pillar (810) | Trap | 30s | Yes (on trigger) | Yes |
| Quagmire (806) | Debuff Zone | 5*Lv sec | No | Yes |
| Volcano (1412) | Buff Zone | 1-5 min | No | Yes |
| Deluge (1413) | Buff Zone | 1-5 min | No | Yes |
| Violent Gale (1414) | Buff Zone | 1-5 min | No | Yes |
| Land Protector (1415) | Anti-Ground Zone | 2-5 min | No | Yes |
| Heaven's Drive (805) | Instant AoE | instant | Yes | No |
| Frost Nova (811) | Instant AoE | instant | Yes | No |

### 5.2 Ground Effect Interactions

- **Land Protector vs all:** LP removes and blocks most ground effects (see Section 4.16 for full list).
- **LP vs LP:** Mutual destruction -- newer LP removes older LP.
- **Volcano prevents:** Ice Wall placement on its area.
- **Violent Gale extends:** Fire Wall duration by +50%.
- **Deluge enables:** Water Ball usage (acts as shallow water). Water Ball consumes Deluge cells.
- **Ice Wall destruction:** Creates water cells for Water Ball.
- **Storm Gust non-stacking:** Two SG on same location = same effect as one. 0.5s immunity.
- **LoV non-stacking:** Same as Storm Gust. 0.5s immunity.
- **Meteor Storm stacking:** Multiple MS CAN stack. Overlapping meteors all deal damage.

### 5.3 Fire Wall Detailed Placement

Fire Wall creates a 1x3 line of fire segments:
- **Perpendicular** to the line from caster to cast point
- Center cell is the cast point, two segments extend to sides
- When cast at caster's own position, wall aligns north-south
- Diagonal walls spawn 2 additional segments to prevent "holes" (total 5 segments for diagonal)
- Each segment has independent hit count (`Lv + 2` hits)
- Each segment processes independently: if one segment expires, others persist

---

## 6. Hindsight / Auto Spell Mechanics

### 6.1 Core Stats

| Property | Value |
|----------|-------|
| ID | 1402 |
| Max Level | 10 |
| SP Cost | 35 (all levels) |
| Cast Time | 3000ms (pre-renewal canonical) |
| Target | Self |
| Prerequisite | Free Cast Lv4 |
| Type | Self-buff |

### 6.2 Per-Level Table

| Lv | Autocast Chance | Duration | Spells Unlocked |
|----|----------------|----------|-----------------|
| 1 | 2% | 120s | Cold Bolt, Fire Bolt, Lightning Bolt (up to Lv3) |
| 2 | 4% | 150s | same |
| 3 | 6% | 180s | same |
| 4 | 8% | 210s | + Soul Strike, Fire Ball |
| 5 | 10% | 240s | same |
| 6 | 12% | 270s | same |
| 7 | 14% | 300s | + Earth Spike, Frost Diver |
| 8 | 16% | 330s | same |
| 9 | 18% | 360s | same |
| 10 | 20% | 390s | + Thunderstorm, Heaven's Drive |

**Note on autocast chance:** Sources disagree on exact percentages. iRO Wiki gives 2-20% in 2% increments. RateMyServer/project docs give 7, 9, 11, 13, 15, 17, 20, 22, 23, 25. The iRO Wiki values are used above as the primary source; the RateMyServer values may reflect a different server configuration. For implementation, the project currently uses the RateMyServer values.

Duration formula: `90 + Lv * 30` seconds

### 6.3 Autocast Spell Level (Pre-Renewal)

Spells cast at **half of Hindsight's skill level**, rounded down. If this is lower than the player's actual learned level of that spell, the actual learned level is used instead.

Example: Hindsight Lv6, player has Fire Bolt Lv10 -> autocast Fire Bolt at Lv10 (since 10 > floor(6/2)=3).

**Alternative (RateMyServer/project docs):** Random bolt level selection:
- 50% = Level 1
- 35% = Level 2
- 15% = Level 3

Cap is the player's learned level.

### 6.4 Autocast Rules

- **SP Cost:** Autocasted spells consume **2/3** (66.7%) of their normal SP cost
- **Cast Time:** None (instant cast)
- **After-Cast Delay:** Present, but does NOT prevent further physical attacks from triggering more autocasts
- **Trigger condition:** Every physical melee attack, whether it hits or misses
- **Spell selection:** Random from available pool at current Hindsight level (must also be learned)
- **Special effects retained:** Freeze from Frost Diver, splash from Fire Ball, etc.

### 6.5 Hindsight Special Properties

- **Undispellable:** Specifically immune to Dispell
- **With Sage Spirit (Soul Linker buff):** Bolts cast at maximum user-learned level instead of half-Hindsight
- Spell cast AFTER the physical attack that triggers it, independent of user's condition

---

## 7. Abracadabra Complete Skill List

### 7.1 Core Stats

| Property | Value |
|----------|-------|
| ID | 1416 |
| Max Level | 10 |
| SP Cost | 50 |
| Cast Time | 0 (instant) |
| Cooldown | 3000ms |
| Target | Self |
| Catalyst | 2 Yellow Gemstones (1 with Mistress Card or Power Cord) |
| Prerequisites | Hindsight Lv5, Land Protector Lv1, Dispell Lv1 |

### 7.2 Regular Skills (Randomly Cast)

Abracadabra randomly selects from ANY player skill at a random skill level. The skill level affects the variety and level of skills that can be cast. The selection pool includes both character and monster skills. The cast skill uses the Sage's own stats for damage calculation.

**Common outcomes include:** Bolt spells, Heal, Blessing, Increase AGI, Cure, Teleport, various offensive and supportive spells from all classes.

### 7.3 Exclusive Effects

These are unique to Abracadabra and cannot be obtained through any other skill:

| Effect | Description | Notes |
|--------|-------------|-------|
| Beastly Hypnosis | Tames target monster (as if taming item used) | Only works on tameable monsters |
| Class Change | Transforms target monster into random MVP | Mini-boss respawn blocked until created MVP dies |
| Coma | Reduces own HP and SP to 1 | Self-cast only |
| Gold Digger | 100% success stealing Zeny from monster | Removed/disabled on most servers |
| Grampus Morph | Changes caster's head to orc appearance | Cosmetic only |
| Gravity | Displays Gravity logo on user's head | Cosmetic / flavor |
| Grim Reaper | Instantly kills target monster | No EXP, no loot. Cannot target bosses |
| Leveling | Grants 10% base EXP | Removed/disabled on most servers |
| Mono Cell | Transforms target monster into Poring | Mini-boss respawn blocked until Poring killed |
| Monster Chant | Summons random monster (Free For All) | Like a Dead Branch effect |
| Questioning | Performs /? emote | Cosmetic only |
| Rejuvenation | Fully restores HP and SP | Self-cast |
| Suicide | Instant self-death with EXP loss | Self-cast |

### 7.4 Implementation Notes

- Disabled in WoE castles
- Exact probabilities are not publicly documented -- implementation typically uses equal weighting for regular skills and very low probability (~1-3% each) for exclusive effects
- The project already has a 145-skill + 6 special effect Abracadabra implementation (see MEMORY.md deferred systems entry)
- For practical gameplay: most casts result in a random offensive or supportive spell; exclusive effects are very rare

---

## 8. Free Cast (Move While Casting) Mechanics

### 8.1 Core Stats

| Property | Value |
|----------|-------|
| ID | 1405 |
| Type | Passive |
| Max Level | 10 |
| Prerequisite | Cast Cancel Lv1 |

### 8.2 Per-Level Table

| Lv | Movement Speed While Casting | ASPD While Casting |
|----|------------------------------|---------------------|
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

### 8.3 Mechanics

- **Movement:** Player can move at reduced speed while casting. Speed = `(25 + 5*Lv)%` of normal.
- **Physical attacks:** Player can attack while casting (important for Battle Sage build with Hindsight).
- **Cast interruption:** Free Cast does NOT prevent damage interruption. When melee attacking while casting, Phen Card or similar is needed to avoid self-interruption from taking damage.
- **ASPD while casting:** `(50 + 5*Lv)%` of normal ASPD. At Lv10, full ASPD while casting.

### 8.4 Server Implementation

Must modify the `player:position` handler:
1. Check `player.learnedSkills[1405] > 0` before calling `interruptCast()`
2. If Free Cast learned: skip `interruptCast()` on movement
3. Apply movement speed reduction during casting
4. Damage still interrupts (unless Phen Card equipped)

---

## 9. Spell Interruption and Phen Card

### 9.1 Default Behavior

All cast-time skills are interruptible:
- **Damage interruption:** Any damage received during casting cancels the cast
- **Movement interruption:** Any movement during casting cancels the cast (unless Free Cast)
- **SP not consumed:** If interrupted, SP is NOT deducted (SP consumed only on successful completion)

### 9.2 Phen Card

| Property | Value |
|----------|-------|
| Card Type | Accessory |
| Effect | Casting cannot be interrupted by damage |
| Penalty | +25% variable cast time |

**Cast time calculation with Phen:**
```
Final Cast = Base * (1 - DEX/150) * (1 + 0.25) * (1 - Suffragium%) * (1 - other reductions%)
```

**Trick:** Equip Phen Card AFTER cast starts. Cast time is calculated only at cast start, so the +25% penalty is avoided while still gaining anti-interruption for the remainder of the cast.

### 9.3 Other Anti-Interruption Effects

| Effect | Source | Notes |
|--------|--------|-------|
| Phen Card | Accessory card | +25% cast time |
| Bloody Butterfly Card | Accessory card | +30% cast time |
| Devotion (Crusader) | Ally absorbs damage | No cast time penalty |
| Endure (Swordsman) | Self-buff | Prevents flinch only, NOT cast interruption in pre-renewal |
| Auto Guard blocking | Crusader passive | When attack is blocked, no interruption |

### 9.4 Skills That Cannot Be Interrupted

Some skills have inherent interruption immunity:
- Redemptio (Priest)
- Skills with 0 cast time (instant)

---

## 10. Skill Trees & Prerequisites

### 10.1 Mage Skill Tree

```
[No prereqs] -----> Cold Bolt (200)
[No prereqs] -----> Fire Bolt (201)
[No prereqs] -----> Lightning Bolt (202)
[No prereqs] -----> Napalm Beat (203)
[No prereqs] -----> Increase SP Recovery (204)
[No prereqs] -----> Sight (205)
[No prereqs] -----> Stone Curse (206)

Fire Bolt Lv4 ----> Fire Ball (207)
Cold Bolt Lv5 ----> Frost Diver (208)
Fire Ball Lv5 + Sight Lv1 ----> Fire Wall (209)
Napalm Beat Lv4 ----> Soul Strike (210)
Napalm Beat Lv7 + Soul Strike Lv5 ----> Safety Wall (211)
Lightning Bolt Lv4 ----> Thunderstorm (212)
[Quest Skill] ----> Energy Coat (213)
```

### 10.2 Wizard Skill Tree

```
Napalm Beat Lv1 + Lightning Bolt Lv1 ----> Jupitel Thunder (800)
Thunderstorm Lv1 + Jupitel Thunder Lv5 ----> Lord of Vermilion (801)
Thunderstorm Lv1 + Sight Rasher Lv2 ----> Meteor Storm (802)
Frost Diver Lv1 + Jupitel Thunder Lv3 ----> Storm Gust (803)
Stone Curse Lv1 ----> Earth Spike (804)
Earth Spike Lv3 ----> Heaven's Drive (805)
Heaven's Drive Lv1 ----> Quagmire (806)
Cold Bolt Lv1 + Lightning Bolt Lv1 ----> Water Ball (807)
Stone Curse Lv1 + Frost Diver Lv1 ----> Ice Wall (808)
Sight Lv1 ----> Sight Rasher (809)
Fire Wall Lv1 ----> Fire Pillar (810)
Frost Diver Lv1 + Ice Wall Lv1 ----> Frost Nova (811)
[No prereqs] ----> Sense (812)
[Quest Skill, Job Lv40] ----> Sight Blaster (813)
```

### 10.3 Sage Skill Tree

```
[No prereqs] ----> Advanced Book (1400)

Advanced Book Lv2 ----> Cast Cancel (1401)
Cast Cancel Lv1 ----> Free Cast (1405)
Free Cast Lv4 ----> Hindsight (1402)

Advanced Book Lv4 ----> Magic Rod (1404)
Magic Rod Lv1 ----> Spell Breaker (1406)
Spell Breaker Lv3 ----> Dispell (1403)

Advanced Book Lv5 + Fire Bolt Lv1 ----> Endow Blaze (1408)
Advanced Book Lv5 + Cold Bolt Lv1 ----> Endow Tsunami (1409)
Advanced Book Lv5 + Lightning Bolt Lv1 ----> Endow Tornado (1410)
Advanced Book Lv5 + Stone Curse Lv1 ----> Endow Quake (1411)

Endow Blaze Lv2 ----> Volcano (1412)
Endow Tsunami Lv2 ----> Deluge (1413)
Endow Tornado Lv2 ----> Violent Gale (1414)
Volcano Lv3 + Deluge Lv3 + Violent Gale Lv3 ----> Land Protector (1415)

Advanced Book Lv9 ----> Dragonology (1407)
Stone Curse Lv1 + Endow Quake Lv1 ----> Earth Spike (1417)
Earth Spike (Sage) Lv1 ----> Heaven's Drive (1418)
[No prereqs] ----> Sense (1419)

Hindsight Lv5 + Land Protector Lv1 + Dispell Lv1 ----> Abracadabra (1416)
[Quest Skill] ----> Create Converter (1420)
[Quest Skill] ----> Elemental Change (1421)
```

---

## 11. Implementation Checklist

### 11.1 Mage Skills -- Status

| ID | Skill | Handler | Data Correct | Status |
|----|-------|---------|-------------|--------|
| 200 | Cold Bolt | YES | YES | COMPLETE |
| 201 | Fire Bolt | YES | YES | COMPLETE |
| 202 | Lightning Bolt | YES | YES | COMPLETE |
| 203 | Napalm Beat | YES | YES | COMPLETE |
| 204 | Inc SP Recovery | YES (partial) | YES | Missing MaxSP% and item potency |
| 205 | Sight | YES | YES | Missing reveal tick |
| 206 | Stone Curse | YES | PARTIAL | Missing 2-phase, HP drain, DEF changes |
| 207 | Fire Ball | YES | YES | AoE radius oversized |
| 208 | Frost Diver | YES | YES | Resistance formula simplified |
| 209 | Fire Wall | YES | YES | Missing 1x3 perpendicular placement |
| 210 | Soul Strike | YES | PARTIAL | ACD table wrong (zigzag) |
| 211 | Safety Wall | YES | YES | Cast time Lv3 typo (3500->3000) |
| 212 | Thunderstorm | YES | PARTIAL | MATK% should be 80, handler uses 100 |
| 213 | Energy Coat | YES | YES | COMPLETE |

### 11.2 Wizard Skills -- Status

| ID | Skill | Handler | Data Correct | Status |
|----|-------|---------|-------------|--------|
| 800 | Jupitel Thunder | YES | YES | COMPLETE |
| 801 | Lord of Vermilion | YES | FIXED | Cast time was inverted, now fixed |
| 802 | Meteor Storm | YES | FIXED | SP/ACD/meteor count fixed |
| 803 | Storm Gust | YES | FIXED | MATK% and freeze mechanic fixed |
| 804 | Earth Spike | YES | YES | COMPLETE |
| 805 | Heaven's Drive | YES | FIXED | SP formula and 125% MATK fixed |
| 806 | Quagmire | YES | FIXED | Reduction amounts, duration, speed fixed |
| 807 | Water Ball | YES | FIXED | SP array and cast time fixed |
| 808 | Ice Wall | YES | FIXED | Duration formula, movement blocking added |
| 809 | Sight Rasher | YES | FIXED | MATK% off-by-20 fixed |
| 810 | Fire Pillar | YES | FIXED | Cast time inverted, now fixed |
| 811 | Frost Nova | YES | FIXED | Cast time array, MATK% formula fixed |
| 812 | Sense | YES | YES | COMPLETE |
| 813 | Sight Blaster | YES | YES | Multi-target reactive trigger added |

### 11.3 Sage Skills -- Status

| ID | Skill | Handler | Data Correct | Status |
|----|-------|---------|-------------|--------|
| 1400 | Advanced Book | YES (passive) | YES | COMPLETE |
| 1401 | Cast Cancel | YES | YES | COMPLETE |
| 1402 | Hindsight | YES | YES | COMPLETE |
| 1403 | Dispell | YES | YES | COMPLETE |
| 1404 | Magic Rod | YES | YES | COMPLETE |
| 1405 | Free Cast | PARTIAL | YES | Movement during cast not enforced |
| 1406 | Spell Breaker | YES | YES | COMPLETE |
| 1407 | Dragonology | YES (passive) | YES | COMPLETE |
| 1408 | Endow Blaze | YES | YES | COMPLETE |
| 1409 | Endow Tsunami | YES | YES | COMPLETE |
| 1410 | Endow Tornado | YES | YES | COMPLETE |
| 1411 | Endow Quake | YES | YES | COMPLETE |
| 1412 | Volcano | YES | YES | COMPLETE |
| 1413 | Deluge | YES | YES | COMPLETE |
| 1414 | Violent Gale | YES | YES | COMPLETE |
| 1415 | Land Protector | YES | YES | COMPLETE |
| 1416 | Abracadabra | YES | YES | COMPLETE (145 skills + 13 exclusive) |
| 1417 | Earth Spike (Sage) | YES | YES | Shares Wizard handler |
| 1418 | Heaven's Drive (Sage) | YES | YES | Shares Wizard handler |
| 1419 | Sense (Sage) | YES | YES | Shares Wizard handler |
| 1420 | Create Converter | YES | YES | COMPLETE |
| 1421 | Elemental Change | YES | YES | COMPLETE |

---

## 12. Gap Analysis vs Current Codebase

### 12.1 Critical Gaps (Game-Breaking if Not Fixed)

| # | Issue | Affected Skills | Priority |
|---|-------|-----------------|----------|
| 1 | Stone Curse missing two-phase petrification | 206 | HIGH |
| 2 | Stone Curse missing HP drain while stoned (1% / 5s until 25%) | 206 | HIGH |
| 3 | Stone Curse missing DEF/MDEF changes (+25% MDEF, -50% DEF) | 206 | HIGH |
| 4 | Thunderstorm handler uses 100% MATK instead of 80% | 212 | MEDIUM |
| 5 | Soul Strike ACD table wrong (linear vs zigzag) | 210 | MEDIUM |
| 6 | Free Cast not enforced (movement during cast not allowed) | 1405 | MEDIUM |

### 12.2 Data Corrections Needed

| Skill | Field | Current | Correct |
|-------|-------|---------|---------|
| Safety Wall (211) | castTime[2] | 3500 | 3000 |
| Soul Strike (210) | ACD array | linear 1200-2800 | [1200,1000,1400,1200,1600,1400,1800,1600,2000,1800] |

### 12.3 Completed Fixes (Previously Resolved)

Per project MEMORY.md and audit docs, the following have been fixed:
- All 14 Wizard skill data corrections (LoV cast time, MS SP/ACD, SG MATK%, HD SP, Quagmire reduction/duration, WB SP/cast, IW duration, SR MATK%, FP cast time, FN cast time/MATK%)
- All 14 Wizard skill handlers implemented
- All 25 Sage skill audit issues (Earth Spike/HD MATK%, zone damage boosts, Volcano ATK, Deluge MaxHP, Hindsight spell pool, Magic Rod absorption, endow expiry, Dispell UNDISPELLABLE set, LP range, etc.)
- Storm Gust freeze mechanic (3rd hit counter, 150% chance)
- Ice Wall movement blocking
- Sight Blaster multi-target reactive trigger
- Quagmire three-layer speed reduction + player blocking
- Ground AoE while-loop catch-up timing (250ms tick)
- Meteor Storm count and interval corrections
- Abracadabra 145 regular + 13 exclusive effects

### 12.4 Systems Still Needing Work

| System | Description | Affected Skills |
|--------|-------------|-----------------|
| Two-phase petrification | Stone Curse needs Petrifying (5s) -> Stone (20s) | 206 |
| Stone HP drain tick | 1% HP / 5s while fully stoned | 206 |
| Stone DEF modifiers | +25% MDEF, -50% DEF while stoned | 206 |
| Free Cast movement | Server must allow movement during cast for Sage | 1405 |
| Water cell tracking | Track water cells for Water Ball hit count | 807 |
| Fire Wall 1x3 placement | Perpendicular wall placement relative to caster | 209 |
| Inc SP Recovery MaxSP% | 0.2%/level of MaxSP per regen tick | 204 |
| Inc SP Recovery item potency | +2%/level to SP recovery items | 204 |
| Sight hidden-reveal tick | Active proximity check for hidden enemies | 205 |

### 12.5 Intentional Deviations

| Deviation | Reason |
|-----------|--------|
| Magic range 900 UE (18 cells) vs canonical 450 (9 cells) | Gameplay feel decision -- larger play area |
| AoE sizes ~2x canonical | Consistent with range scaling |
| Endow failure not breaking weapon | Too punishing without repair NPC |
| Simplified freeze/stone resistance (no LUK/level diff) | Minor impact in PvE; update for PvP |

---

## Research Sources

- [Fire Bolt - iRO Wiki](https://irowiki.org/wiki/Fire_Bolt)
- [Cold Bolt - iRO Wiki](https://irowiki.org/wiki/Cold_Bolt)
- [Mage - iRO Wiki](https://irowiki.org/wiki/Mage)
- [Storm Gust - iRO Wiki](https://irowiki.org/wiki/Storm_Gust)
- [Storm Gust - iRO Wiki Classic](https://irowiki.org/classic/Storm_Gust)
- [Meteor Storm - iRO Wiki](https://irowiki.org/wiki/Meteor_Storm)
- [Meteor Storm - iRO Wiki Classic](https://irowiki.org/classic/Meteor_Storm)
- [Lord of Vermilion - iRO Wiki](https://irowiki.org/wiki/Lord_of_Vermilion)
- [Fire Wall - iRO Wiki Classic](https://irowiki.org/classic/Fire_Wall)
- [Fire Wall - Ragnarok Research Lab](https://ragnarokresearchlab.github.io/game-mechanics/effects/fire-wall/)
- [Ice Wall - iRO Wiki](https://irowiki.org/wiki/Ice_Wall)
- [Ice Wall - iRO Wiki Classic](https://irowiki.org/classic/Ice_Wall)
- [Quagmire - iRO Wiki Classic](https://irowiki.org/classic/Quagmire)
- [Quagmire - iRO Wiki](https://irowiki.org/wiki/Quagmire)
- [Hindsight - iRO Wiki](https://irowiki.org/wiki/Hindsight)
- [Free Cast - iRO Wiki](https://irowiki.org/wiki/Free_Cast)
- [Hocus-pocus - iRO Wiki](https://irowiki.org/wiki/Hocus-pocus)
- [Hocus-pocus - iRO Wiki Classic](https://irowiki.org/classic/Hocus-pocus)
- [Magic Rod - iRO Wiki](https://irowiki.org/wiki/Magic_Rod)
- [Dispel - iRO Wiki](https://irowiki.org/wiki/Dispel)
- [Volcano - iRO Wiki](https://irowiki.org/wiki/Volcano)
- [Deluge - iRO Wiki](https://irowiki.org/wiki/Deluge)
- [Stone Curse - iRO Wiki](https://irowiki.org/wiki/Stone_Curse)
- [Stone Curse - iRO Wiki Classic](https://irowiki.org/classic/Stone_Curse)
- [Sage - iRO Wiki Classic](https://irowiki.org/classic/Sage)
- [Wizard Skills - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=9)
- [Sage Skills - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=16)
- [Mage Skills - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=2)
- [Soul Strike - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=13)
- [Spell Breaker - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=277)
- [Hocus-pocus - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=290)
- [rAthena pre-re skill_db.txt](https://github.com/flaviojs/rathena-commits/blob/master/db/pre-re/skill_db.txt)
- [rAthena skill.conf](https://github.com/rathena/rathena/blob/master/conf/battle/skill.conf)
- [Wizard Pre-Renewal - pservero](https://pre.pservero.com/job/Wizard/)
- [Phen Card - RateMyServer Pre-Re](http://ratemyserver.net/index.php?item_id=4077&page=item_db)
- [Energy Coat - iRO Wiki](https://irowiki.org/wiki/Energy_Coat)
- [Safety Wall - iRO Wiki](https://irowiki.org/wiki/Safety_Wall)
- [Casting Time/Interruption Cards - Ragnarok Wiki](https://ragnarok.fandom.com/wiki/Casting_Time/Interruption_Cards)
- [Wizard Builds - ROClassicGuide](https://roclassic-guide.com/wizard-builds-for-pre-renewal-and-classic/)
- [Ragnarok Online Calculator Pre-renewal](http://calc.free-ro.com/)
