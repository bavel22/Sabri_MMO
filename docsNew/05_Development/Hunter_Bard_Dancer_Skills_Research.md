# Hunter, Bard & Dancer Skills — Pre-Renewal Complete Reference

**Research Date:** 2026-03-11
**Sources:** iRO Wiki (classic + renewal), RateMyServer, rAthena pre-renewal DB, Ragnarok Fandom Wiki, GameFAQs guides, rAthena GitHub issues
**Scope:** Base 2nd class skills ONLY (no transcendent/Sniper/Minstrel/Gypsy-exclusive skills)

---

## Table of Contents

1. [Hunter Skills (19 skills)](#hunter-skills)
2. [Bard Skills (12 skills + 8 ensemble)](#bard-skills)
3. [Dancer Skills (12 skills + 8 ensemble)](#dancer-skills)
4. [Ensemble Skills (8 shared)](#ensemble-skills)
5. [Skill Name Cross-Reference (iRO vs kRO)](#name-cross-reference)
6. [Soul Link Skills (excluded — requires Soul Linker)](#soul-link-notes)

---

## Hunter Skills

Hunter inherits all Archer skills (Owl's Eye, Vulture's Eye, Improve Concentration, Double Strafe, Arrow Shower, Arrow Crafting, Arrow Repel).
Hunter has 19 of its own skills: 12 traps, 3 passives, 2 falcon skills, 2 quest/platinum skills.

### HT-01: Skid Trap (ID: 115)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Trap) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **SP Cost** | 10 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | None |
| **Required Item** | 1 Trap |
| **Description** | Places a trap that knocks back the triggering unit. |

| Level | Knockback (cells) | Trap Duration (sec) |
|-------|--------------------|---------------------|
| 1 | 6 | 300 |
| 2 | 7 | 240 |
| 3 | 8 | 180 |
| 4 | 9 | 120 |
| 5 | 10 | 60 |

**Special Mechanics:**
- Knockback = 5 + SkillLv cells
- Trap duration = (5 - SkillLv + 1) * 60 seconds
- Trap becomes visible after placement (not hidden)
- Does no damage

---

### HT-02: Land Mine (ID: 116)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Trap) |
| **Target Type** | Ground |
| **Element** | Earth |
| **Range** | 3 cells |
| **SP Cost** | 10 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | None |
| **Required Item** | 1 Trap |
| **Description** | Earth-property trap dealing DEX/INT-based damage with stun chance. |

| Level | Damage Formula | Stun Chance | Stun Duration | Trap Duration (sec) |
|-------|---------------|-------------|---------------|---------------------|
| 1 | (DEX+75) * (1+INT/100) * 1 | 35% | 5s | 200 |
| 2 | (DEX+75) * (1+INT/100) * 2 | 40% | 5s | 160 |
| 3 | (DEX+75) * (1+INT/100) * 3 | 45% | 5s | 120 |
| 4 | (DEX+75) * (1+INT/100) * 4 | 50% | 5s | 80 |
| 5 | (DEX+75) * (1+INT/100) * 5 | 55% | 5s | 40 |

**Special Mechanics:**
- Damage is MISC type (ignores DEF/MDEF, not affected by cards)
- Stun chance = 5 * SkillLv + 30
- Affected by Earth element modifiers on target
- AoE: hits only the triggering cell (1x1)

---

### HT-03: Ankle Snare (ID: 117)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Trap) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **SP Cost** | 12 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Skid Trap Lv 1 |
| **Required Item** | 1 Trap |
| **Description** | Immobilizes the target that steps on it. Duration scales with level. |

| Level | Snare Duration (sec) | Trap Duration (sec) |
|-------|---------------------|---------------------|
| 1 | 4 | 250 |
| 2 | 8 | 200 |
| 3 | 12 | 150 |
| 4 | 16 | 100 |
| 5 | 20 | 50 |

**Special Mechanics:**
- Actual snare duration on players: 5 * SkillLv / (TargetAGI * 0.1), minimum 3 seconds
- Boss monsters: snare time = SkillLv * 3 seconds (capped, shorter)
- Target cannot move but can use skills and attack
- Does no damage
- Trap becomes visible when triggered

---

### HT-04: Shockwave Trap (ID: 118)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Trap) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **AoE** | 3x3 cells |
| **SP Cost** | 45 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Ankle Snare Lv 1 |
| **Required Item** | 2 Traps |
| **Description** | Drains SP from targets in a 3x3 area. |

| Level | SP Drain % | Trap Duration (sec) |
|-------|-----------|---------------------|
| 1 | 20% | 200 |
| 2 | 35% | 160 |
| 3 | 50% | 120 |
| 4 | 65% | 80 |
| 5 | 80% | 40 |

**Special Mechanics:**
- Drains percentage of target's current SP
- Does not affect Boss monsters
- Does no HP damage

---

### HT-05: Sandman (ID: 119)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Trap) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **AoE** | 5x5 cells |
| **SP Cost** | 12 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Flasher Lv 1 |
| **Required Item** | 1 Trap |
| **Description** | Puts targets in a 5x5 area to sleep. |

| Level | Sleep Chance | Sleep Duration (sec) | Trap Duration (sec) |
|-------|-------------|---------------------|---------------------|
| 1 | 50% | 30 | 150 |
| 2 | 60% | 30 | 120 |
| 3 | 70% | 30 | 90 |
| 4 | 80% | 30 | 60 |
| 5 | 90% | 30 | 30 |

**Special Mechanics:**
- Sleep breaks on damage
- Does not affect Boss monsters
- Sleep chance = 50 + SkillLv * 10 - 10

---

### HT-06: Flasher (ID: 120)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Trap) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **AoE** | 3x3 cells |
| **SP Cost** | 12 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Skid Trap Lv 1 |
| **Required Item** | 2 Traps |
| **Description** | Inflicts Blind on targets in a 3x3 area. |

| Level | Blind Duration (sec) | Trap Duration (sec) |
|-------|---------------------|---------------------|
| 1 | 30 | 150 |
| 2 | 30 | 120 |
| 3 | 30 | 90 |
| 4 | 30 | 60 |
| 5 | 30 | 30 |

**Special Mechanics:**
- Blind reduces HIT and FLEE
- Does not affect Boss monsters
- Blind duration is always 30 seconds regardless of level (level affects trap duration only)

---

### HT-07: Freezing Trap (ID: 121)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Trap) |
| **Target Type** | Ground |
| **Element** | Water |
| **Range** | 3 cells |
| **AoE** | 3x3 cells |
| **SP Cost** | 10 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Flasher Lv 1 |
| **Required Item** | 2 Traps |
| **Description** | Deals Water-property damage and freezes targets. |

| Level | Damage % ATK | Freeze Duration (sec) | Trap Duration (sec) |
|-------|-------------|----------------------|---------------------|
| 1 | 50% | 3 | 150 |
| 2 | 75% | 6 | 120 |
| 3 | 100% | 9 | 90 |
| 4 | 125% | 12 | 60 |
| 5 | 150% | 15 | 30 |

**Special Mechanics:**
- Damage formula: (25 + 25 * SkillLv)% of ATK
- Freeze makes target Water element (vulnerable to Wind)
- Does not freeze Boss monsters (still deals damage)
- Freeze duration = 3 * SkillLv seconds

---

### HT-08: Blast Mine (ID: 122)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Trap) |
| **Target Type** | Ground |
| **Element** | Wind |
| **Range** | 3 cells |
| **AoE** | 3x3 cells |
| **SP Cost** | 10 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Land Mine Lv 1, Sandman Lv 1, Freezing Trap Lv 1 |
| **Required Item** | 1 Trap |
| **Description** | Wind-property trap that auto-detonates after a timer. |

| Level | Damage Formula | Auto-Detonate Timer (sec) |
|-------|---------------|---------------------------|
| 1 | (50+DEX/2) * (1+INT/100) * 1 | 25 |
| 2 | (50+DEX/2) * (1+INT/100) * 2 | 20 |
| 3 | (50+DEX/2) * (1+INT/100) * 3 | 15 |
| 4 | (50+DEX/2) * (1+INT/100) * 4 | 10 |
| 5 | (50+DEX/2) * (1+INT/100) * 5 | 5 |

**Special Mechanics:**
- MISC type damage (ignores DEF/MDEF)
- Auto-detonates even if no target steps on it
- Can also be detonated by stepping on it before timer expires
- Wind element modifier applies

---

### HT-09: Claymore Trap (ID: 123)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Trap) |
| **Target Type** | Ground |
| **Element** | Fire |
| **Range** | 3 cells |
| **AoE** | 5x5 cells |
| **SP Cost** | 15 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Shockwave Trap Lv 1, Blast Mine Lv 1 |
| **Required Item** | 2 Traps |
| **Description** | Powerful Fire-property trap with large AoE. |

| Level | Damage Formula | Trap Duration (sec) |
|-------|---------------|---------------------|
| 1 | (75+DEX/2) * (1+INT/100) * 1 | 20 |
| 2 | (75+DEX/2) * (1+INT/100) * 2 | 40 |
| 3 | (75+DEX/2) * (1+INT/100) * 3 | 60 |
| 4 | (75+DEX/2) * (1+INT/100) * 4 | 80 |
| 5 | (75+DEX/2) * (1+INT/100) * 5 | 100 |

**Special Mechanics:**
- MISC type damage (ignores DEF/MDEF)
- Largest AoE trap (5x5)
- Fire element modifier applies
- Duration INCREASES with level (opposite of most traps)

---

### HT-10: Remove Trap (ID: 124)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Utility) |
| **Target Type** | Trap (on ground) |
| **Element** | Neutral |
| **Range** | 2 cells |
| **SP Cost** | 5 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Land Mine Lv 1 |
| **Description** | Removes an active trap and recovers 1 Trap item. |

**Special Mechanics:**
- Can only remove your own traps (or traps from same party in some versions)
- Returns 1 Trap item to inventory
- Cannot remove triggered/activated traps

---

### HT-11: Talkie Box (ID: 125)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Trap/Utility) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **Range** | 3 cells |
| **SP Cost** | 1 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Shockwave Trap Lv 1, Remove Trap Lv 1 |
| **Required Item** | 1 Trap |
| **Description** | Places a trap that displays a custom message when triggered. |

**Special Mechanics:**
- User inputs message at cast time
- Trap lasts 600 seconds (10 minutes)
- Purely utility — no damage or status effect
- Message visible to all nearby players

---

### HT-12: Spring Trap (ID: 131)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Utility) |
| **Target Type** | Trap (on ground) |
| **Element** | Neutral |
| **SP Cost** | 10 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Remove Trap Lv 1, Falconry Mastery Lv 1 |
| **Requires** | Falcon equipped |
| **Description** | Remotely destroys a visible trap from range. |

| Level | Range (cells) |
|-------|--------------|
| 1 | 4 |
| 2 | 5 |
| 3 | 6 |
| 4 | 7 |
| 5 | 8 |

**Special Mechanics:**
- Can target any visible trap (enemy or own)
- Does NOT recover Trap item (unlike Remove Trap)
- Requires falcon to be equipped
- Range increases with level

---

### HT-13: Beast Bane (ID: 126)

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Passive |
| **Target Type** | None |
| **Element** | Neutral |
| **Prerequisites** | None |
| **Description** | Increases ATK against Brute and Insect race monsters. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|-----|
| **ATK Bonus** | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |

**Special Mechanics:**
- Flat ATK bonus (not percentage)
- Formula: 4 * SkillLv
- Only applies vs Brute and Insect race
- Required for Falconry Mastery

---

### HT-14: Falconry Mastery (ID: 127)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Passive |
| **Target Type** | None |
| **Element** | Neutral |
| **Prerequisites** | Beast Bane Lv 1 |
| **Description** | Allows the Hunter to rent a falcon from the Hunter Guild. |

**Special Mechanics:**
- Enables falcon rental (100 Zeny from Hunter Guild NPC)
- Falcon enables: Blitz Beat, Detect, Spring Trap, auto-Blitz Beat
- Cannot have falcon and cart at same time
- Cannot have falcon and Peco Peco at same time (Crusader mount)

---

### HT-15: Steel Crow (ID: 128)

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Passive |
| **Target Type** | None |
| **Element** | Neutral |
| **Prerequisites** | Blitz Beat Lv 5 |
| **Description** | Increases falcon damage for Blitz Beat. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|-----|
| **Falcon ATK Bonus** | +6 | +12 | +18 | +24 | +30 | +36 | +42 | +48 | +54 | +60 |

**Special Mechanics:**
- Formula: 6 * SkillLv per Blitz Beat hit
- Only affects Blitz Beat damage (both manual and auto)

---

### HT-16: Blitz Beat (ID: 129)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive |
| **Target Type** | Single (with 3x3 AoE splash) |
| **Element** | Neutral |
| **Range** | 5 + Vulture's Eye level (cells) |
| **Cast Time** | 1500 ms (1200 ms variable + 300 ms fixed) |
| **After-Cast Delay** | 1000 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Falconry Mastery Lv 1 |
| **Requires** | Falcon equipped |
| **Description** | Command falcon to attack. Number of hits increases with level. |

| Level | Hits | SP Cost |
|-------|------|---------|
| 1 | 1 | 10 |
| 2 | 2 | 13 |
| 3 | 3 | 16 |
| 4 | 4 | 19 |
| 5 | 5 | 22 |

**Damage Formula (Pre-Renewal):**
```
Per-hit damage = (80 + SteelCrow_Lv * 6 + floor(INT/2) * 2 + floor(DEX/10) * 2)
Total damage = Per-hit * NumberOfHits
```

**SP Cost Formula:** 7 + (SkillLv * 3)

**Auto-Blitz Beat (Passive trigger):**
- Triggers on normal attacks with falcon equipped
- Trigger chance: LUK / 3 (%) — e.g., 30 LUK = 10% chance
- Auto-Blitz always uses Lv 1 damage (unless job level 40+, then scales with job level: JLv1-9=Lv1, JLv10-19=Lv2, JLv20-29=Lv3, JLv30-39=Lv4, JLv40+=Lv5)
- Auto-Blitz consumes NO SP
- Auto-Blitz can trigger even on missed attacks
- Auto-Blitz damage is split among targets in 3x3 area (manual Blitz Beat is NOT split)

---

### HT-17: Detect (ID: 130)

| Field | Value |
|-------|-------|
| **Max Level** | 4 |
| **Type** | Active (Utility) |
| **Target Type** | Ground |
| **Element** | Neutral |
| **AoE** | 7x7 cells |
| **SP Cost** | 8 (all levels) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Improve Concentration Lv 1, Falconry Mastery Lv 1 |
| **Requires** | Falcon equipped |
| **Description** | Reveals hidden/cloaked targets and invisible traps. |

| Level | Range (cells) |
|-------|--------------|
| 1 | 3 |
| 2 | 5 |
| 3 | 7 |
| 4 | 9 |

**Special Mechanics:**
- Reveals Hiding, Cloaking, and Chase Walk
- Also reveals invisible traps
- 7x7 detection area centered on target cell
- Range increases with level

---

### HT-18: Phantasmic Arrow (ID: 1009) — Quest/Platinum Skill

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Offensive |
| **Target Type** | Single |
| **Element** | Neutral (or arrow element if equipped) |
| **Range** | 9 cells |
| **SP Cost** | 10 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Quest completion (Hunter Platinum Skill Quest, Job Lv 40+) |
| **Requires** | Bow equipped |
| **Description** | Fires a phantom arrow that requires no ammunition and knocks back target. |

**Damage:** 150% ATK
**Knockback:** 3 cells

**Special Mechanics:**
- Does NOT consume arrows
- Creates a phantom arrow for the attack
- Uses arrow element if arrows are equipped, otherwise Neutral
- Knockback pushes target 3 cells away from caster

---

### HT-19: Beast Strafing / Beast Charge (ID: 499) — Soul Link Skill

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Offensive |
| **Target Type** | Single |
| **Element** | Weapon element |
| **Range** | 9 cells |
| **SP Cost** | 12 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 100 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Double Strafe Lv 10, Beast Bane Lv 10 |
| **Requires** | Bow equipped, Arrow equipped, Hunter Spirit (Soul Link), must use immediately after Double Strafe |
| **Description** | Enhanced attack against Brute/Insect targets. Only usable under Hunter Spirit. |

**Damage Formula (Pre-Renewal):** (50 + STR * 8)% ATK
**Target restriction:** Brute or Insect race monsters ONLY

> **NOTE:** This is a soul link skill — requires a Soul Linker to cast Hunter Spirit on the user. May be excluded from initial implementation.

---

## Bard Skills

Bards inherit all Archer skills. Bards equip Instruments as their special weapon (replacing bow for performances).
Bards have 12 unique skills: 1 passive, 5 utility/offensive, 4 performance songs, 1 AoE freeze, 1 quest skill.

### Shared Bard/Dancer Skills

#### BD-01: Amp / Adaptation to Circumstances (ID: 304)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Utility) |
| **Target Type** | Self |
| **Element** | Neutral |
| **Range** | 0 |
| **SP Cost** | 1 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | None |
| **Description** | Cancels the current active song/dance/ensemble performance. |

**Special Mechanics:**
- Cannot be used within 5 seconds of starting a performance
- Preferred method: switch to dagger to cancel instantly (no 5s delay)
- Both Bard and Dancer learn this skill

---

#### BD-02: Encore (ID: 305)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Utility) |
| **Target Type** | Self |
| **Element** | Neutral |
| **Range** | 0 |
| **SP Cost** | 1 |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 10000 ms |
| **Prerequisites** | Amp Lv 1 |
| **Requires** | Instrument (Bard) or Whip (Dancer) |
| **Description** | Repeats the last performed song/dance at 50% SP cost. |

**Special Mechanics:**
- Uses half the SP of the original performance
- 10 second cooldown between uses
- Both Bard and Dancer learn this skill

---

### Bard-Only Skills

#### BA-01: Music Lessons (ID: 315)

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Passive |
| **Target Type** | None |
| **Element** | Neutral |
| **Prerequisites** | None |
| **Description** | Increases instrument ATK, movement speed while singing, and improves song effects. |

| Level | Instrument ATK | Voice ATK | Move Speed in Song | Song Effect Bonus |
|-------|---------------|-----------|--------------------|--------------------|
| 1 | +3 | +3 | +5% | Improves all song formulas |
| 2 | +6 | +6 | +10% | |
| 3 | +9 | +9 | +15% | |
| 4 | +12 | +12 | +20% | |
| 5 | +15 | +15 | +25% | |
| 6 | +18 | +18 | +30% | |
| 7 | +21 | +21 | +35% | |
| 8 | +24 | +24 | +40% | |
| 9 | +27 | +27 | +45% | |
| 10 | +30 | +30 | +50% | |

**Special Mechanics:**
- Affects most Bard song formulas (adds to FLEE in A Whistle, ASPD in Assassin Cross, Cast reduction in Bragi, etc.)
- Move speed bonus applies only during active performance
- Required for Melody Strike and Dissonance

---

#### BA-02: Melody Strike / Musical Strike (ID: 316)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Physical) |
| **Target Type** | Single |
| **Element** | Weapon/Arrow property |
| **Range** | 9 cells |
| **Cast Time** | 1500 ms (pre-renewal) |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Music Lessons Lv 3 |
| **Requires** | Instrument equipped, 1 Arrow |
| **Description** | Ranged physical attack using instrument. |

| Level | Damage % ATK | SP Cost |
|-------|-------------|---------|
| 1 | 100% | 1 |
| 2 | 140% | 3 |
| 3 | 180% | 5 |
| 4 | 220% | 7 |
| 5 | 260% | 9 |

**Pre-Renewal Damage Formula:** (60 + SkillLv * 40)% ATK
**Pre-Renewal SP Cost:** (SkillLv * 2) - 1

**Special Mechanics:**
- Can be used while singing
- Requires instrument weapon
- Consumes 1 arrow per use
- Uses arrow element for damage property

---

#### BA-03: Unchained Serenade / Dissonance (ID: 317)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Magic) — Performance |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 9x9 cells (pre-renewal: 7x7) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 5000 ms |
| **Prerequisites** | Music Lessons Lv 1, Amp Lv 1 |
| **Requires** | Instrument equipped |
| **Description** | Performance that deals Neutral magic damage every 3 seconds to enemies in AoE. |

| Level | Damage Per Tick | SP Cost (pre-renewal) |
|-------|----------------|----------------------|
| 1 | 20 + MusicLessons*3 | 18 |
| 2 | 40 + MusicLessons*3 | 21 |
| 3 | 60 + MusicLessons*3 | 24 |
| 4 | 80 + MusicLessons*3 | 27 |
| 5 | 100 + MusicLessons*3 | 30 |

**Pre-Renewal SP Cost:** 15 + SkillLv * 3
**Duration:** 30 seconds
**SP Drain:** 1 SP per 3 seconds while active

**Special Mechanics:**
- Non-elemental magic damage — ignores DEF, reduced by MDEF
- Damage tick every 3 seconds
- Music Lessons adds +3 to +10 per tick (3 * ceil(MusicLessons/3))
- PvP/WoE only in some servers; usable everywhere in pre-renewal
- Required at Lv 3 for all 4 main Bard songs

---

#### BA-04: Unbarring Octave / Frost Joker (ID: 318)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active |
| **Target Type** | Self (screen-wide AoE) |
| **Element** | Neutral |
| **AoE** | Screen-wide |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 4000 ms |
| **Prerequisites** | Encore Lv 1 |
| **Description** | Tells a terrible joke — chance to freeze everyone on screen (including party). |

| Level | Freeze Chance | SP Cost |
|-------|-------------|---------|
| 1 | 20% | 12 |
| 2 | 25% | 14 |
| 3 | 30% | 16 |
| 4 | 35% | 18 |
| 5 | 40% | 20 |

**Special Mechanics:**
- Affects EVERYONE on screen (enemies AND party members)
- Does not affect Boss monsters
- Freeze duration: 5 seconds on players, longer on monsters
- Bard class counterpart to Dancer's Dazzler/Scream
- Cast delay in pre-renewal was 3-4 seconds

---

#### BA-05: Perfect Tablature / A Whistle (ID: 319) — Performance Song

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Unchained Serenade Lv 3 |
| **Requires** | Instrument equipped |
| **Description** | Song that increases FLEE and Perfect Dodge of allies in AoE. |

**Pre-Renewal Formula:**
```
FLEE Boost = SkillLv + floor(AGI/10) + MusicLessons_Lv
Perfect Dodge Boost = floor(SkillLv/2) + floor(LUK/10) + ceil(MusicLessons_Lv/2)
```

| Level | Base FLEE | Base PD | SP Cost (pre-renewal) |
|-------|----------|---------|----------------------|
| 1 | +1 | +1 | 22 |
| 2 | +2 | +1 | 24 |
| 3 | +3 | +2 | 26 |
| 4 | +4 | +2 | 28 |
| 5 | +5 | +3 | 30 |
| 6 | +6 | +3 | 32 |
| 7 | +7 | +4 | 34 |
| 8 | +8 | +4 | 36 |
| 9 | +9 | +5 | 38 |
| 10 | +10 | +5 | 40 |

**Duration:** 60 seconds (pre-renewal)
**SP Drain:** 1 SP per 5 seconds while active

**Special Mechanics:**
- Bard must stay in position while performing (reduced movement speed applies)
- Effect lingers ~20 seconds after leaving AoE ("flashing" mechanic)
- Only affects caster and party members
- Cannot stack with other performance songs (only 1 active at a time)

---

#### BA-06: Impressive Riff / Assassin Cross of Sunset (ID: 320) — Performance Song

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Unchained Serenade Lv 3 |
| **Requires** | Instrument equipped |
| **Description** | Song that increases ASPD of allies in AoE. |

**Pre-Renewal Formula:**
```
ASPD Boost = (10 + SkillLv + floor(AGI/10) + ceil(MusicLessons_Lv/2)) %
```

| Level | Base ASPD Boost | SP Cost (pre-renewal) |
|-------|----------------|----------------------|
| 1 | 11% | 38 |
| 2 | 12% | 41 |
| 3 | 13% | 44 |
| 4 | 14% | 47 |
| 5 | 15% | 50 |
| 6 | 16% | 53 |
| 7 | 17% | 56 |
| 8 | 18% | 59 |
| 9 | 19% | 62 |
| 10 | 20% | 65 |

**Pre-Renewal SP Cost:** 35 + (SkillLv * 3)
**Duration:** 120 seconds (pre-renewal)
**SP Drain:** 1 SP per 3 seconds while active

**Special Mechanics:**
- Stacks with Speed Potions but NOT with Adrenaline Rush
- AGI adds +1% per 10 points, Music Lessons adds +1% per 2 levels
- Pre-renewal uses direct ASPD% boost, NOT the after-cast delay reduction of post-renewal

---

#### BA-07: Magic Strings / A Poem of Bragi (ID: 321) — Performance Song

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Unchained Serenade Lv 3 |
| **Requires** | Instrument equipped |
| **Description** | Song that reduces cast time and after-cast delay of allies in AoE. |

**Pre-Renewal Formulas:**
```
VCT Reduction = (SkillLv * 3 + floor(DEX/10) + MusicLessons_Lv) %
After-Cast Delay Reduction = (SkillLv * 3 + floor(SkillLv/10) * 20 + floor(INT/5) + MusicLessons_Lv * 2) %
```

| Level | Base VCT Reduction | Base Delay Reduction | SP Cost (pre-renewal) |
|-------|--------------------|---------------------|-----------------------|
| 1 | -3% | -3% | 65 |
| 2 | -6% | -6% | 70 |
| 3 | -9% | -9% | 75 |
| 4 | -12% | -12% | 80 |
| 5 | -15% | -15% | 85 |
| 6 | -18% | -18% | 90 |
| 7 | -21% | -21% | 95 |
| 8 | -24% | -24% | 100 |
| 9 | -27% | -27% | 105 |
| 10 | -30% | -30% | 110 |

**Duration:** 180 seconds (pre-renewal)
**SP Drain:** 1 SP per 5 seconds while active

**Special Mechanics:**
- One of the most powerful support skills in the game
- DEX contributes to VCT reduction, INT contributes to delay reduction
- Music Lessons improves both reductions
- Only affects party members in AoE

---

#### BA-08: Song of Lutie / Apple of Idun (ID: 322) — Performance Song

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Unchained Serenade Lv 3 |
| **Requires** | Instrument equipped |
| **Description** | Song that increases MaxHP and HP recovery of allies in AoE. |

**Pre-Renewal Formula:**
```
MaxHP Boost = (5 + SkillLv * 2 + floor(VIT/10) + MusicLessons_Lv) %
HP Recovery Per Tick = 35 + SkillLv * 5 + floor(VIT/10) * 5
```

| Level | Base MaxHP Boost | HP Recovery/Tick | SP Cost |
|-------|-----------------|-----------------|---------|
| 1 | +7% | 40 | 40 |
| 2 | +9% | 45 | 45 |
| 3 | +11% | 50 | 50 |
| 4 | +13% | 55 | 55 |
| 5 | +15% | 60 | 60 |
| 6 | +17% | 65 | 65 |
| 7 | +19% | 70 | 70 |
| 8 | +21% | 75 | 75 |
| 9 | +23% | 80 | 80 |
| 10 | +25% | 85 | 85 |

**Duration:** 180 seconds (pre-renewal)
**SP Drain:** 1 SP per 6 seconds while active

**Special Mechanics:**
- MaxHP boost formula includes VIT and Music Lessons bonus
- Also provides periodic HP recovery every few seconds
- Only affects party members in AoE

---

#### BA-09: Pang Voice (ID: 1010) — Quest/Platinum Skill

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active |
| **Target Type** | Single |
| **Element** | Neutral |
| **Range** | 9 cells |
| **SP Cost** | 20 |
| **Cast Time** | 800 ms |
| **After-Cast Delay** | 2000 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Quest completion (Bard Platinum Skill Quest, Job Lv 40+) |
| **Description** | Shout at a target to inflict Chaos (Confusion) status. |

**Special Mechanics:**
- Inflicts Chaos (Confusion) status
- Does not work on Boss monsters
- Success rate affected by target's INT
- Duration: ~15 seconds
- No weapon restriction

---

## Dancer Skills

Dancers inherit all Archer skills. Dancers equip Whips as their special weapon.
Dancers have 12 unique skills: 1 passive, 5 utility/offensive, 4 performance dances, 1 AoE stun, 1 quest skill.

### Dancer-Only Skills

#### DC-01: Dance Lessons (ID: 323)

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Passive |
| **Target Type** | None |
| **Element** | Neutral |
| **Prerequisites** | None |
| **Description** | Increases whip ATK, movement speed while dancing, and improves dance effects. |

| Level | Whip ATK | Move Speed in Dance | Dance Effect Bonus |
|-------|---------|--------------------|--------------------|
| 1 | +3 | +5% | Improves all dance formulas |
| 2 | +6 | +10% | |
| 3 | +9 | +15% | |
| 4 | +12 | +20% | |
| 5 | +15 | +25% | |
| 6 | +18 | +30% | |
| 7 | +21 | +35% | |
| 8 | +24 | +40% | |
| 9 | +27 | +45% | |
| 10 | +30 | +50% | |

**Special Mechanics:**
- Mirror of Bard's Music Lessons
- Affects most Dancer dance formulas
- Required for Slinging Arrow and Hip Shaker

---

#### DC-02: Slinging Arrow / Throw Arrow (ID: 324)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Offensive (Physical) |
| **Target Type** | Single |
| **Element** | Weapon/Arrow property |
| **Range** | 9 cells |
| **Cast Time** | 1500 ms (pre-renewal) |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Dance Lessons Lv 3 |
| **Requires** | Whip equipped, 1 Arrow |
| **Description** | Ranged physical attack using whip to sling an arrow. |

| Level | Damage % ATK | SP Cost |
|-------|-------------|---------|
| 1 | 100% | 1 |
| 2 | 140% | 3 |
| 3 | 180% | 5 |
| 4 | 220% | 7 |
| 5 | 260% | 9 |

**Pre-Renewal Damage Formula:** (60 + SkillLv * 40)% ATK
**Pre-Renewal SP Cost:** (SkillLv * 2) - 1

**Special Mechanics:**
- Mirror of Bard's Melody Strike
- Can be used while dancing
- Requires whip weapon and arrows

---

#### DC-03: Hip Shaker / Ugly Dance (ID: 325)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 5000 ms |
| **Prerequisites** | Dance Lessons Lv 1, Amp Lv 1 |
| **Requires** | Whip equipped |
| **Description** | Dance that drains SP from enemies in AoE every 3 seconds. |

| Level | SP Drain Per Tick | SP Cost (pre-renewal) |
|-------|------------------|----------------------|
| 1 | 8 + DanceLessons*6 | 23 |
| 2 | 16 + DanceLessons*6 | 26 |
| 3 | 24 + DanceLessons*6 | 29 |
| 4 | 32 + DanceLessons*6 | 32 |
| 5 | 40 + DanceLessons*6 | 35 |

**Pre-Renewal SP Cost:** 20 + SkillLv * 3
**Duration:** 30 seconds
**SP Drain (caster):** 1 SP per 3 seconds while active

**Special Mechanics:**
- Mirror of Bard's Unchained Serenade (but drains SP instead of dealing damage)
- Dance Lessons increases drain amount
- PvP/WoE focused but usable in PvE in pre-renewal
- Required at Lv 3 for all 4 main Dancer dances

---

#### DC-04: Dazzler / Scream (ID: 326)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active |
| **Target Type** | Self (screen-wide AoE) |
| **Element** | Neutral |
| **AoE** | Screen-wide |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 4000 ms |
| **Prerequisites** | Encore Lv 1 |
| **Description** | Screams loudly — chance to stun everyone on screen (including party). |

| Level | Stun Chance | SP Cost |
|-------|-----------|---------|
| 1 | 30% | 12 |
| 2 | 35% | 14 |
| 3 | 40% | 16 |
| 4 | 45% | 18 |
| 5 | 50% | 20 |

**Stun Chance Formula:** 25 + SkillLv * 5
**Stun Duration:** ~5 seconds

**Special Mechanics:**
- Affects EVERYONE on screen (enemies AND party members)
- Does not affect Boss monsters
- Dancer class counterpart to Bard's Frost Joker
- Cast delay in pre-renewal was 3-4 seconds

---

#### DC-05: Focus Ballet / Humming (ID: 327) — Performance Dance

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Hip Shaker Lv 3 |
| **Requires** | Whip equipped |
| **Description** | Dance that increases HIT of allies in AoE. |

**Pre-Renewal Formula:**
```
HIT Boost = SkillLv * 2 + floor(DEX/10) + DanceLessons_Lv
```

| Level | Base HIT Boost | SP Cost (pre-renewal) |
|-------|---------------|----------------------|
| 1 | +2 | 22 |
| 2 | +4 | 24 |
| 3 | +6 | 26 |
| 4 | +8 | 28 |
| 5 | +10 | 30 |
| 6 | +12 | 32 |
| 7 | +14 | 34 |
| 8 | +16 | 36 |
| 9 | +18 | 38 |
| 10 | +20 | 40 |

**Pre-Renewal SP Cost:** 20 + (SkillLv * 2)
**Duration:** 60 seconds (pre-renewal)
**SP Drain:** 1 SP per 5 seconds while active

**Special Mechanics:**
- DEX adds +1 HIT per 10 points
- Dance Lessons adds additional HIT
- Only affects caster and party members in AoE
- Dancer counterpart to Bard's Perfect Tablature

---

#### DC-06: Slow Grace / Please Don't Forget Me (ID: 328) — Performance Dance

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Hip Shaker Lv 3 |
| **Requires** | Whip equipped |
| **Description** | Dance that reduces ASPD and movement speed of enemies in AoE. |

**Pre-Renewal Formulas:**
```
ASPD Reduction = (5 + SkillLv * 3 + floor(DEX/10) + DanceLessons_Lv) %
Movement Speed Reduction = (5 + SkillLv * 3 + floor(AGI/10) + DanceLessons_Lv) %
```

| Level | Base ASPD Reduction | Base MoveSpeed Reduction | SP Cost (pre-renewal) |
|-------|--------------------|--------------------------|-----------------------|
| 1 | -8% | -8% | 38 |
| 2 | -11% | -11% | 41 |
| 3 | -14% | -14% | 44 |
| 4 | -17% | -17% | 47 |
| 5 | -20% | -20% | 50 |
| 6 | -23% | -23% | 53 |
| 7 | -26% | -26% | 56 |
| 8 | -29% | -29% | 59 |
| 9 | -32% | -32% | 62 |
| 10 | -35% | -35% | 65 |

**Duration:** 180 seconds (pre-renewal)
**SP Drain:** 1 SP per 10 seconds while active

**Special Mechanics:**
- Cancels ASPD-boosting buffs on affected targets
- Dancer counterpart to Bard's Impressive Riff (one buffs, one debuffs)
- PvP/WoE focused in renewal; usable everywhere in pre-renewal
- DEX contributes to ASPD reduction, AGI to movement reduction

---

#### DC-07: Lady Luck / Fortune's Kiss (ID: 329) — Performance Dance

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Hip Shaker Lv 3 |
| **Requires** | Whip equipped |
| **Description** | Dance that increases Critical rate of allies in AoE. |

**Pre-Renewal Formula:**
```
CRIT Boost = SkillLv + floor(LUK/10) + DanceLessons_Lv
```

| Level | Base CRIT Boost | SP Cost |
|-------|----------------|---------|
| 1 | +1 | 40 |
| 2 | +2 | 45 |
| 3 | +3 | 50 |
| 4 | +4 | 55 |
| 5 | +5 | 60 |
| 6 | +6 | 65 |
| 7 | +7 | 70 |
| 8 | +8 | 75 |
| 9 | +9 | 80 |
| 10 | +10 | 85 |

**Duration:** 120 seconds (pre-renewal)
**SP Drain:** 1 SP per 4 seconds while active

**Special Mechanics:**
- LUK adds +1 CRIT per 10 points
- Dance Lessons adds +1 per level
- Only affects caster and party members in AoE
- Dancer counterpart to Bard's Magic Strings

---

#### DC-08: Gypsy's Kiss / Service For You (ID: 330) — Performance Dance

| Field | Value |
|-------|-------|
| **Max Level** | 10 |
| **Type** | Active (Performance) |
| **Target Type** | Self (AoE around caster) |
| **Element** | Neutral |
| **AoE** | 7x7 cells (pre-renewal) |
| **Cast Time** | 0 ms |
| **After-Cast Delay** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Hip Shaker Lv 3 |
| **Requires** | Whip equipped |
| **Description** | Dance that increases MaxSP and reduces SP consumption of allies in AoE. |

**Pre-Renewal Formulas:**
```
MaxSP Boost = (15 + SkillLv + floor(INT/10)) %
SP Cost Reduction = (20 + SkillLv * 3 + floor(INT/10) + ceil(DanceLessons_Lv/2)) %
```

| Level | Base MaxSP Boost | Base SP Reduction | SP Cost |
|-------|-----------------|------------------|---------|
| 1 | +16% | -23% | 60 |
| 2 | +17% | -26% | 63 |
| 3 | +18% | -29% | 66 |
| 4 | +19% | -32% | 69 |
| 5 | +20% | -35% | 72 |
| 6 | +21% | -38% | 75 |
| 7 | +22% | -41% | 78 |
| 8 | +23% | -44% | 81 |
| 9 | +24% | -47% | 84 |
| 10 | +25% | -50% | 87 |

**Duration:** 180 seconds (pre-renewal)
**SP Drain:** 1 SP per 5 seconds while active

**Special Mechanics:**
- INT contributes to both MaxSP boost and SP reduction
- Dance Lessons contributes to SP reduction
- Only affects caster and party members in AoE
- Dancer counterpart to Bard's Song of Lutie

---

#### DC-09: Charming Wink / Wink of Charm (ID: 1011) — Quest/Platinum Skill

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active |
| **Target Type** | Single |
| **Element** | Neutral |
| **Range** | 9 cells |
| **SP Cost** | 20 |
| **Cast Time** | 800 ms |
| **After-Cast Delay** | 2000 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Quest completion (Dancer Platinum Skill Quest, Job Lv 40+) |
| **Description** | Wink at a target to inflict Confusion and Hallucination. |

**Special Mechanics:**
- Inflicts Confusion and Hallucination status
- Does not work on Boss monsters
- No weapon restriction (usable with any weapon)
- Duration: ~10 seconds
- Dancer counterpart to Bard's Pang Voice

---

## Ensemble Skills

Ensemble skills require BOTH a Bard and Dancer in the same party, standing adjacent to each other.
The skill level performed equals the LOWER level between the Bard's and Dancer's relevant skill.
Both performers are locked in place during the ensemble.

### EN-01: Lullaby (ID: 306)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **SP Cost** | 20 |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Perfect Tablature Lv 10 (Bard) / Focus Ballet Lv 10 (Dancer) |
| **Description** | Puts enemies in AoE to sleep. |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 4 seconds while active

**Special Mechanics:**
- Sleep breaks on damage
- Does not affect Boss monsters
- INT of both performers affects success rate
- Both performers must stay adjacent

---

### EN-02: Into the Abyss / Power Cord (ID: 312)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **SP Cost** | 10 |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Lullaby Lv 1 |
| **Description** | Party members can use skills without gemstone/catalyst requirements. |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 5 seconds while active

**Special Mechanics:**
- Only affects party members (not the performers themselves)
- Removes gemstone cost for skills like Safety Wall, Resurrection, etc.
- Does NOT remove Trap item cost for Hunter skills

---

### EN-03: Classical Pluck / Loki's Wail (ID: 311)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **SP Cost** | 15 |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Impressive Riff Lv 10 (Bard) / Slow Grace Lv 10 (Dancer) |
| **Description** | Disables ALL skill usage in the AoE except for the performers. |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 4 seconds while active

**Special Mechanics:**
- Affects EVERYONE in the AoE (enemies and allies)
- Only the performing Bard and Dancer can use skills
- PvP/WoE focused
- Very powerful zone control skill

---

### EN-04: Down Tempo / Eternal Chaos (ID: 308)

| Field | Value |
|-------|-------|
| **Max Level** | 1 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **SP Cost** | 30 |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Classical Pluck Lv 1 |
| **Description** | Reduces VIT-based DEF of all enemies in AoE to 0. |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 4 seconds while active

**Special Mechanics:**
- Only reduces soft DEF (VIT component), not equipment DEF
- Does not affect party members
- PvP/WoE focused

---

### EN-05: Acoustic Rhythm / Invulnerable Siegfried (ID: 313)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Magic Strings Lv 10 (Bard) / Lady Luck Lv 10 (Dancer) |
| **Description** | Increases elemental resistance and status ailment resistance. |

| Level | Elemental Resistance | Status Resistance | SP Cost |
|-------|---------------------|-------------------|---------|
| 1 | +3% | +5% | 20 |
| 2 | +6% | +10% | 20 |
| 3 | +9% | +15% | 20 |
| 4 | +12% | +20% | 20 |
| 5 | +15% | +25% | 20 |

**Note:** Some sources report higher values at max level (up to 80% elemental, 50% status) — this may be the formula with stat bonuses included.

**Duration:** 60 seconds
**SP Drain:** 1 SP per 3 seconds while active

---

### EN-06: Mental Sensing / Mr. Kim A Rich Man (ID: 307)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Acoustic Rhythm Lv 3 |
| **Description** | Increases EXP gained from monsters killed in AoE. |

| Level | EXP Bonus | SP Cost |
|-------|----------|---------|
| 1 | +20% | 20 |
| 2 | +30% | 20 |
| 3 | +40% | 20 |
| 4 | +50% | 20 |
| 5 | +60% | 20 |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 3 seconds while active

**Special Mechanics:**
- Monster must be killed inside the AoE
- Stacks with other EXP modifiers (Battle Manual, etc.)

---

### EN-07: Battle Theme / A Drum on the Battlefield (ID: 309)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Song of Lutie Lv 10 (Bard) / Gypsy's Kiss Lv 10 (Dancer) |
| **Description** | Increases ATK and DEF of party members in AoE. |

| Level | ATK Bonus | DEF Bonus | SP Cost |
|-------|----------|----------|---------|
| 1 | +50 | +4 | 20 |
| 2 | +75 | +6 | 20 |
| 3 | +100 | +8 | 20 |
| 4 | +125 | +10 | 20 |
| 5 | +150 | +12 | 20 |

**Note:** Some sources report different values (e.g., +20/+15 at Lv1 to +40/+75 at Lv5). The values from the blogspot guide reference pre-renewal values which tend to be higher than the RateMyServer listing.

**Duration:** 60 seconds
**SP Drain:** 1 SP per 3 seconds while active

**Special Mechanics:**
- Only affects party members
- ATK is flat bonus, not percentage

---

### EN-08: Harmonic Lick / The Ring of Nibelungen (ID: 310)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Active (Ensemble) |
| **Target Type** | Self (AoE around performers) |
| **Element** | Neutral |
| **AoE** | 9x9 cells |
| **Cast Time** | 0 ms |
| **Cooldown** | 0 ms |
| **Prerequisites** | Battle Theme Lv 3 |
| **Description** | Grants large ATK bonus to party members wielding Level 4 weapons. |

| Level | ATK Bonus (Lv4 weapons only) | SP Cost |
|-------|------------------------------|---------|
| 1 | +150 | 20 |
| 2 | +200 | 20 |
| 3 | +250 | 20 |
| 4 | +300 | 20 |
| 5 | +350 | 20 |

**Duration:** 60 seconds
**SP Drain:** 1 SP per 3 seconds while active

**Special Mechanics:**
- ONLY benefits party members wielding Level 4 weapons
- Level 1-3 weapons receive no bonus
- Very powerful but restricted in utility
- Some sources report random buff effects instead of flat ATK

---

## Name Cross-Reference

The Bard/Dancer skill naming is notoriously confusing due to iRO localization. Here is the complete mapping:

### Bard Skills
| Skill ID | kRO / Alt Name | iRO Name | Internal (rAthena) |
|----------|---------------|----------|---------------------|
| 304 | Adaptation to Circumstances | Amp | BA_MUSICALLESSON (shared) |
| 305 | Encore | Encore | BA_MUSICALSTRIKE (shared) |
| 315 | Musical Lesson | Music Lessons | BA_MUSICALLESSON |
| 316 | Musical Strike | Melody Strike | BA_MUSICALSTRIKE |
| 317 | Dissonance | Unchained Serenade | BA_DISSONANCE |
| 318 | Frost Joker | Unbarring Octave | BA_FROSTJOKER |
| 319 | A Whistle | Perfect Tablature | BA_WHISTLE |
| 320 | Assassin Cross of Sunset | Impressive Riff | BA_ASSASSINCROSS |
| 321 | A Poem of Bragi | Magic Strings | BA_POEMBRAGI |
| 322 | The Apple of Idun | Song of Lutie | BA_APPLEIDUN |
| 1010 | Pang Voice | Pang Voice | BA_PANGVOICE |

### Dancer Skills
| Skill ID | kRO / Alt Name | iRO Name | Internal (rAthena) |
|----------|---------------|----------|---------------------|
| 304 | Adaptation to Circumstances | Amp | (shared with Bard) |
| 305 | Encore | Encore | (shared with Bard) |
| 323 | Dancing Lesson | Dance Lessons | DC_DANCINGLESSON |
| 324 | Throw Arrow | Slinging Arrow | DC_THROWARROW |
| 325 | Ugly Dance | Hip Shaker | DC_UGLYDANCE |
| 326 | Scream | Dazzler | DC_SCREAM |
| 327 | Humming | Focus Ballet | DC_HUMMING |
| 328 | Please Don't Forget Me | Slow Grace | DC_DONTFORGETME |
| 329 | Fortune's Kiss | Lady Luck | DC_FORTUNEKISS |
| 330 | Service for You | Gypsy's Kiss | DC_SERVICEFORYOU |
| 1011 | Wink of Charm | Charming Wink | DC_WINKCHARM |

### Ensemble Skills
| Skill ID | kRO / Alt Name | iRO Name | Internal (rAthena) |
|----------|---------------|----------|---------------------|
| 306 | Lullaby | Lullaby | BD_LULLABY |
| 307 | Mr. Kim A Rich Man | Mental Sensing | BD_RICHMANKIM |
| 308 | Eternal Chaos | Down Tempo | BD_ETERNALCHAOS |
| 309 | A Drum on the Battlefield | Battle Theme | BD_DRUMBATTLEFIELD |
| 310 | The Ring of Nibelungen | Harmonic Lick | BD_RINGNIBELUNGEN |
| 311 | Loki's Wail | Classical Pluck | BD_LOKISWAIL |
| 312 | Into the Abyss | Power Cord | BD_INTOABYSS |
| 313 | Invulnerable Siegfried | Acoustic Rhythm | BD_SIEGFRIED |

---

## Soul Link Notes (EXCLUDED from base implementation)

The following are **NOT base 2nd class skills** — they require Soul Linker's "Bard and Dancer Spirits" buff:

**When a Bard is soul-linked, they can use:**
- Slow Grace (Dancer's debuff dance) if they have Impressive Riff Lv 10
- Lady Luck (Dancer's crit dance) if they have Magic Strings Lv 10
- Focus Ballet (Dancer's HIT dance) if they have Perfect Tablature Lv 10
- Gypsy's Kiss (Dancer's SP dance) if they have Song of Lutie Lv 10

**When a Dancer is soul-linked, they can use:**
- Impressive Riff (Bard's ASPD song) if they have Slow Grace Lv 10
- Magic Strings (Bard's cast reduction song) if they have Lady Luck Lv 10
- Perfect Tablature (Bard's FLEE song) if they have Focus Ballet Lv 10
- Song of Lutie (Bard's HP song) if they have Gypsy's Kiss Lv 10

**Beast Strafing / Beast Charge (ID: 499)** — Hunter soul link skill, also excluded.

These should be deferred to a future "Soul Linker" implementation phase.

---

## Performance Skill Mechanics (Shared Rules)

All Bard songs and Dancer dances follow these common rules in pre-renewal:

1. **One performance at a time** — Cannot have 2 songs/dances active simultaneously
2. **Movement restriction** — Performer moves at reduced speed (Music/Dance Lessons helps)
3. **Weapon requirement** — Bard needs Instrument, Dancer needs Whip
4. **AoE follows caster** — The 7x7 area moves with the performer
5. **SP drain** — Each performance drains SP periodically while active
6. **Flashing/Lingering** — Effects persist ~20 seconds after targets leave the AoE
7. **Cancel methods** — Amp skill (5s delay), switch to dagger (instant), or wait for duration
8. **Ensemble lock** — During ensemble, both performers are rooted in place
9. **Ensemble level** — Uses the LOWER skill level between the two performers
10. **Stat scaling** — Pre-renewal formulas include performer stats (AGI, DEX, INT, LUK, VIT) and lesson skill level

---

## Skill Count Summary

| Class | Solo Skills | Quest Skills | Ensemble | Total |
|-------|------------|-------------|----------|-------|
| **Hunter** | 17 | 1 (Phantasmic Arrow) | 0 | 18 (+1 soul link) |
| **Bard** | 10 | 1 (Pang Voice) | 8 (shared) | 11 (+8 ensemble) |
| **Dancer** | 10 | 1 (Charming Wink) | 8 (shared) | 11 (+8 ensemble) |

---

## Sources

- [iRO Wiki Classic - Hunter](https://irowiki.org/classic/Hunter)
- [iRO Wiki Classic - Bard](https://irowiki.org/classic/Bard)
- [iRO Wiki Classic - Dancer](https://irowiki.org/classic/Dancer)
- [iRO Wiki - Blitz Beat](https://irowiki.org/wiki/Blitz_Beat)
- [iRO Wiki - Magic Strings](https://irowiki.org/wiki/Magic_Strings)
- [iRO Wiki - Impressive Riff](https://irowiki.org/wiki/Impressive_Riff)
- [iRO Wiki - Song of Lutie](https://irowiki.org/wiki/Song_of_Lutie)
- [iRO Wiki - Perfect Tablature](https://irowiki.org/wiki/Perfect_Tablature)
- [iRO Wiki - Slow Grace](https://irowiki.org/wiki/Slow_Grace)
- [iRO Wiki - Lady Luck](https://irowiki.org/wiki/Lady_Luck)
- [iRO Wiki - Gypsy's Kiss](https://irowiki.org/wiki/Gypsy%27s_Kiss)
- [iRO Wiki - Bard and Dancer Spirits](https://irowiki.org/wiki/Bard_and_Dancer_Spirits)
- [RateMyServer - Hunter Skills](https://ratemyserver.net/index.php?page=skill_db&jid=11)
- [RateMyServer - Bard Skills](https://ratemyserver.net/index.php?page=skill_db&jid=19)
- [RateMyServer - Dancer Skills](https://ratemyserver.net/index.php?page=skill_db&jid=20)
- [Ragnarok Fandom Wiki - Bard](https://ragnarok.fandom.com/wiki/Bard)
- [Ragnarok Fandom Wiki - Dancer](https://ragnarok.fandom.com/wiki/Dancer)
- [GameFAQs - Bard/Dancer/Clown/Gypsy Guide](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/35759)
- [My Life in Ragnarok - Bard/Dancer Guide](http://myrolife.blogspot.com/2016/05/barddancerclowngypsy-guide.html)
- [rAthena GitHub - Skill Balance Issues](https://github.com/rathena/rathena/issues/3715)
- [rAthena GitHub - Apple of Idun Scaling](https://github.com/rathena/rathena/issues/7727)
- [rAthena GitHub - Assassin Cross Base Values](https://github.com/rathena/rathena/issues/7726)
- [rAthena GitHub - Slow Grace Discrepancies](https://github.com/rathena/rathena/issues/7747)
