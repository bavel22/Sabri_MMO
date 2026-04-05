# ASPD, Hit, Flee & Critical -- Deep Research (Pre-Renewal)

> Comprehensive reference for Ragnarok Online Classic (pre-Renewal) attack speed, accuracy, evasion, and critical hit mechanics.
> Sources: rAthena pre-renewal source (`status.cpp`, `battle.cpp`, `db/pre-re/job_aspd.yml`), iRO Wiki Classic, RateMyServer, Hercules emulator, community research.
> Cross-referenced against Sabri_MMO server implementation in `server/src/ro_damage_formulas.js` and `server/src/ro_exp_tables.js`.

---

## Table of Contents

1. [ASPD Calculation](#1-aspd-calculation)
2. [HIT System](#2-hit-system)
3. [FLEE System](#3-flee-system)
4. [Critical System](#4-critical-system)
5. [Edge Cases](#5-edge-cases)
6. [Implementation Checklist](#6-implementation-checklist)
7. [Gap Analysis](#7-gap-analysis)

---

## 1. ASPD Calculation

ASPD (Attack Speed) determines how frequently a character can perform auto-attacks. It ranges from 0 to 190, where 190 is the hard cap allowing 5 attacks per second.

### 1.1 Core ASPD Formula

The pre-renewal ASPD formula from iRO Wiki Classic:

```
ASPD = 200 - (WD - floor((WD * AGI / 25) + (WD * DEX / 100)) / 10) * (1 - SM)
```

Where:
- **WD** = Weapon Delay = `BaseASPD` value from the job/weapon table (see Section 1.2)
- **AGI** = Character's Agility stat (post-buff effective value)
- **DEX** = Character's Dexterity stat (post-buff effective value)
- **SM** = Speed Modifier = sum of skill + potion ASPD modifiers (see Section 1.4)
- `floor()` brackets = round to nearest integer (some sources say truncate)

**Breakdown of stat contribution:**
- AGI provides ~4x the ASPD benefit of DEX per point (`WD * AGI / 25` vs `WD * DEX / 100`)
- It takes roughly 4-10 AGI to increase ASPD by 1 point (varies by weapon delay)
- DEX contribution is marginal for ASPD; its primary role is HIT

**Alternative (iRO Aegis-accurate) formula** found in some sources:

```
ASPD Penalty = floor((1 - (Job Base ASPD - 144) / 50) * 100) / 100   (max 0.96)
ASPD Correction = ceil((sqrt(205) - sqrt(AGI)) / 7.15 * 1000) / 1000
Base ASPD = floor((200 - (200 - (Job Base ASPD + Shield Penalty - ASPD Correction
           + sqrt(AGI * 9.999 + DEX * 0.19212) * ASPD Penalty))
           * (1 - Potion ASPD Mod - Skill ASPD Mod)) * 100) / 100
Final ASPD = floor(Base ASPD + Equip ASPD % + Equip ASPD Fixed)
```

The simpler WD-based formula is the rAthena/Hercules standard and is what most private servers and our implementation use. The Aegis formula is more complex but produces nearly identical results.

### 1.2 Base ASPD by Class and Weapon Type (Complete Table)

Values from rAthena `db/pre-re/job_aspd.yml`. These are the `BaseASPD` (weapon delay) values -- higher = slower. Default for unequippable weapons is 2000 (effectively unusable).

#### First Classes

| Weapon Type | Novice | Swordsman | Mage | Archer | Acolyte | Merchant | Thief |
|-------------|--------|-----------|------|--------|---------|----------|-------|
| Fist        | 500    | 400       | 500  | 400    | 400     | 400      | 400   |
| Dagger      | 650    | 500       | 600  | 600    | --      | 600      | 500   |
| 1H Sword    | 700    | 550       | --   | --     | --      | 700      | 650   |
| 2H Sword    | --     | 600       | --   | --     | --      | --       | --    |
| 1H Spear    | --     | 650       | --   | --     | --      | --       | --    |
| 2H Spear    | --     | 700       | --   | --     | --      | --       | --    |
| 1H Axe      | 800    | 700       | --   | --     | --      | 700      | 800   |
| 2H Axe      | --     | 750       | --   | --     | --      | 750      | --    |
| Mace        | 700    | 650       | --   | --     | 600     | 700      | --    |
| 2H Mace     | 700    | 700       | --   | --     | 600     | 700      | --    |
| Staff       | 650    | --        | 700  | --     | 600     | --       | --    |
| 2H Staff    | 650    | --        | 700  | --     | 600     | --       | --    |
| Bow         | --     | --        | --   | 700    | --      | --       | 800   |
| Knuckle     | --     | --        | --   | --     | --      | --       | --    |
| Instrument   | --     | --        | --   | --     | --      | --       | --    |
| Whip        | --     | --        | --   | --     | --      | --       | --    |
| Book        | --     | --        | --   | --     | --      | --       | --    |
| Katar       | --     | --        | --   | --     | --      | --       | --    |

#### Second Classes

| Weapon Type | Knight | Crusader | Assassin | Hunter | Wizard | Priest | Monk |
|-------------|--------|----------|----------|--------|--------|--------|------|
| Fist        | 400    | 400      | 400      | 400    | 500    | 400    | 400  |
| Dagger      | 500    | 500      | 500      | 600    | 575    | --     | --   |
| 1H Sword    | 500    | 500      | 650      | --     | --     | --     | --   |
| 2H Sword    | 550    | 550      | --       | --     | --     | --     | --   |
| 1H Spear    | 600    | 600      | --       | --     | --     | --     | --   |
| 2H Spear    | 600    | 600      | --       | --     | --     | --     | --   |
| 1H Axe      | 700    | 700      | 800      | --     | --     | --     | --   |
| 2H Axe      | 700    | 700      | --       | --     | --     | --     | --   |
| Mace        | 650    | 650      | --       | --     | --     | 600    | 575  |
| 2H Mace     | 700    | 700      | --       | --     | --     | 600    | 575  |
| Staff       | --     | --       | --       | --     | 625    | 600    | 575  |
| 2H Staff    | --     | --       | --       | --     | 625    | 600    | 575  |
| Bow         | --     | --       | --       | 600    | --     | --     | --   |
| Knuckle     | --     | --       | --       | --     | --     | --     | 475  |
| Book        | --     | --       | --       | --     | --     | 600    | --   |
| Katar       | --     | --       | 500      | --     | --     | --     | --   |

| Weapon Type | Sage | Blacksmith | Alchemist | Rogue | Bard | Dancer |
|-------------|------|------------|-----------|-------|------|--------|
| Fist        | 450  | 400        | 400       | 400   | 400  | 400    |
| Dagger      | 525  | 600        | 550       | 500   | 550  | 550    |
| 1H Sword    | --   | 650        | 575       | 550   | --   | --     |
| 1H Axe      | --   | 650        | 675       | --    | --   | --     |
| 2H Axe      | --   | 650        | 700       | --    | --   | --     |
| Mace        | --   | 675        | 650       | --    | --   | --     |
| 2H Mace     | --   | 675        | 650       | --    | --   | --     |
| Staff       | 625  | --         | --        | --    | --   | --     |
| 2H Staff    | 625  | --         | --        | --    | --   | --     |
| Bow         | --   | --         | --        | 650   | 650  | 650    |
| Book        | 550  | --         | --        | --    | --   | --     |
| Instrument  | --   | --         | --        | --    | 575  | --     |
| Whip        | --   | --         | --        | --    | --   | 575    |

**Reading the table:** A Knight with a 2H Sword has BaseASPD = 550. With 90 AGI, 30 DEX, and no speed buffs:

```
WD = 550
AGI reduction = floor(550 * 90 / 25) = floor(1980) = 1980
DEX reduction = floor(550 * 30 / 100) = floor(165) = 165
Total reduction = floor((1980 + 165) / 10) = floor(214.5) = 214
ASPD = 200 - floor((550 - 214) * (1 - 0)) = 200 - 336 = ... wait, that gives negative.
```

**Important clarification on units:** The rAthena `job_aspd.yml` `BaseASPD` values are in **centiseconds** (1/100 second), not the BTBA values used in the iRO Wiki formula. The conversion is:

```
BTBA (seconds) = BaseASPD / 100     (e.g., 550 -> 5.50 seconds base delay)
WD (iRO formula) = BaseASPD / 10    (e.g., 550 -> 55 weapon delay units)
```

Using the iRO formula with `WD = BaseASPD / 10`:

```
WD = 550 / 10 = 55
AGI reduction = floor(55 * 90 / 25) = floor(198) = 198
DEX reduction = floor(55 * 30 / 100) = floor(16.5) = 16
Total reduction = floor((198 + 16) / 10) = floor(21.4) = 21
ASPD = 200 - floor((55 - 21) * 1) = 200 - 34 = 166
```

This yields ASPD 166 for a Knight with 2H Sword, 90 AGI, 30 DEX, which is reasonable (0.68s between attacks, ~1.47 hits/sec).

### 1.3 AGI and DEX Contribution Formula

From the core formula, the stat reduction to weapon delay is:

```
StatReduction = floor((floor(WD * AGI / 25) + floor(WD * DEX / 100)) / 10)
```

**AGI contribution per point:**
- At WD=50 (fast weapons): 1 AGI = 2.0 delay reduction per 25 = 0.08 per AGI point
- At WD=70 (bows): 1 AGI = 2.8 delay reduction per 25 = 0.112 per AGI point
- Practical: ~4-10 AGI per 1 ASPD point depending on weapon speed

**DEX contribution per point:**
- At WD=50: 1 DEX = 0.5 delay reduction per 100 = 0.005 per DEX point
- At WD=70: 1 DEX = 0.7 delay reduction per 100 = 0.007 per DEX point
- Practical: ~14-20 DEX per 1 ASPD point depending on weapon speed
- DEX is roughly 4x less efficient than AGI for ASPD

**Breakpoint behavior:** ASPD increases in discrete steps. Between breakpoints, adding AGI or DEX has no visible effect. This creates the "AGI breakpoint" system where players optimize stat builds around these thresholds.

### 1.4 ASPD Buffs and Potions

Speed Modifiers (SM) stack additively but potions use only the highest value (no potion stacking). The total SM is applied as `(1 - SM)` to the base delay.

#### Potions (Highest Only -- Do Not Stack With Each Other)

| Potion | SM Value | Job Restriction |
|--------|----------|-----------------|
| Concentration Potion | 0.10 (10%) | All classes |
| Awakening Potion | 0.15 (15%) | Swordsman, Merchant, Thief and 2nd classes |
| Berserk Potion | 0.20 (20%) | Swordsman, Merchant, Thief and 2nd classes |
| Poison Bottle (EDP active) | 0.25 (25%) | Assassin Cross only |

#### Skills (Stack With Potions)

| Skill | SM Value | Condition |
|-------|----------|-----------|
| Two-Hand Quicken (Knight) | 0.30 | 2H Sword equipped |
| One-Hand Quicken (Crusader/Knight) | 0.30 | 1H Sword equipped (Soul Link or Crusader) |
| Spear Quicken (Crusader) | 0.21-0.30 (varies by level) | 2H Spear equipped |
| Adrenaline Rush (Blacksmith, self) | 0.30 | Axe or Mace equipped |
| Adrenaline Rush (Blacksmith, party) | 0.25 | Party members with Axe/Mace |
| Full Adrenaline Rush (Whitesmith) | 0.30 | Any weapon type |
| Frenzy / Berserk (Lord Knight) | 0.30 | Any weapon (also locks HP at full) |

**Skill mutual exclusion (ASPD Haste2 group):**
- Two-Hand Quicken, One-Hand Quicken, Spear Quicken, Adrenaline Rush, and Assassin Cross of Sunset all belong to the same exclusion group
- Only the strongest active buff applies; activating a new one cancels the previous
- Potions stack with the active skill buff

#### Equipment Modifiers

| Equipment / Card | SM Value |
|-----------------|----------|
| Doppelganger Card | 0.10 |
| Berserk Guitar [Instrument] | 1.00 (instant cap) |
| Thunder P [Knuckle] | 0.20 |

#### Negative Modifiers (Reduce ASPD)

| Source | SM Value |
|--------|----------|
| Peco Peco Riding (no Cavalry Mastery) | -0.50 |
| Doom Slayer [2H Sword] | -0.40 |

**Peco Peco mount penalty:**
- Base mount penalty: 50% ASPD reduction
- Cavalry Mastery restores 10% per level (Lv5 = full restore, no penalty)
- Formula: `ASPD_mounted = floor(ASPD * (0.5 + CavalryMasteryLv * 0.1))`

#### Combined SM Example

A Knight with Two-Hand Quicken (0.30) + Berserk Potion (0.20):
```
SM = 0.30 + 0.20 = 0.50
Delay factor = (1 - 0.50) = 0.50 (halved delay)
```

### 1.5 ASPD Cap (190) and Soft Cap Mechanics

**Hard cap: 190 ASPD**
- The maximum achievable ASPD in pre-Renewal is **190**
- This corresponds to 0.2 seconds between attacks (5 hits per second)
- The formula approaches 200 asymptotically but is clamped at 190

**No soft cap in official pre-Renewal:**
- Unlike Renewal (which has diminishing returns above 190), pre-Renewal has a flat hard cap
- All excess ASPD above 190 is wasted

**Sabri_MMO deviation:** Our implementation allows ASPD up to 195 with diminishing returns above 190, producing a minimum interval of ~217ms (~4.6 attacks/sec). This is a deliberate design choice documented in `server/src/index.js`.

### 1.6 Attack Delay Conversion

The conversion from ASPD to actual delay between attacks:

```
Attack Delay (seconds) = (200 - ASPD) / 50
Attack Delay (ms)      = (200 - ASPD) * 20
Hits per Second         = 50 / (200 - ASPD)
```

**Alternative formula (some sources):**
```
Amotion (ms) = (200 - ASPD) * 10
ASPD display = (2000 - Amotion) / 10
```

The `* 20` vs `* 10` discrepancy comes from different internal representations. rAthena uses centisecond units internally (amotion in ms / 10), while the display formula converts back. The `(200 - ASPD) * 20` formula gives the correct millisecond delay.

#### Reference Table

| ASPD | Delay (ms) | Delay (s) | Hits/sec |
|------|-----------|-----------|----------|
| 100  | 2000      | 2.00      | 0.50     |
| 110  | 1800      | 1.80      | 0.56     |
| 120  | 1600      | 1.60      | 0.63     |
| 130  | 1400      | 1.40      | 0.71     |
| 140  | 1200      | 1.20      | 0.83     |
| 150  | 1000      | 1.00      | 1.00     |
| 155  | 900       | 0.90      | 1.11     |
| 160  | 800       | 0.80      | 1.25     |
| 165  | 700       | 0.70      | 1.43     |
| 170  | 600       | 0.60      | 1.67     |
| 175  | 500       | 0.50      | 2.00     |
| 180  | 400       | 0.40      | 2.50     |
| 183  | 340       | 0.34      | 2.94     |
| 185  | 300       | 0.30      | 3.33     |
| 187  | 260       | 0.26      | 3.85     |
| 189  | 220       | 0.22      | 4.55     |
| 190  | 200       | 0.20      | 5.00     |

### 1.7 Amotion and Dmotion

- **Amotion** = Attack animation duration in milliseconds. Directly tied to ASPD.
  ```
  Amotion_ms = (200 - ASPD) * 20
  ```
- **Dmotion** = Damage motion / hit stun duration. A fixed value per monster/class that causes brief inaction after being hit. Used for hitlock calculations.

**Hitlock condition:** A monster is hitlocked (cannot move) when attacks arrive faster than its flinch recovery allows movement. The condition is:

```
For some positive integer x:
(x * HitDelay - dMotion) < Speed / 2
```

Where `HitDelay` = attacker's attack interval in ms, `dMotion` = target's flinch duration, `Speed` = target's movement speed in ms/cell.

### 1.8 Dual Wielding ASPD (Assassin/Assassin Cross)

Assassins (and Assassin Cross) can dual-wield daggers, 1H swords, and 1H axes. The combined weapon delay is:

```
DualBTBA = 0.7 * (BTBA_RightHand + BTBA_LeftHand)
```

Or equivalently with BaseASPD values:
```
WD_dual = floor((BaseASPD_Right + BaseASPD_Left) * 7 / 10)
```

**Example:** Assassin dual-wielding two daggers (BaseASPD 500 each):
```
WD_dual = floor((500 + 500) * 7 / 10) = floor(700) = 700
```

**Key properties:**
- Both hands attack each cycle, dealing damage independently
- Right hand applies full card mods; left hand applies its own weapon cards
- Skills use right-hand weapon only
- The combined WD is used for the ASPD calculation (higher combined delay = slower)

### 1.9 Shield Penalty

Equipping a shield reduces ASPD. In rAthena pre-renewal:
- Shield penalty is a flat 5-10 reduction added to the weapon delay
- The exact value is configured per shield type in the item database
- Most shields add +5 to weapon delay; heavy shields add up to +10
- In the iRO formula, shield penalty is incorporated directly into the base ASPD calculation

---

## 2. HIT System

HIT (accuracy) determines whether a physical attack connects. Magic always hits (ignoring FLEE entirely).

### 2.1 HIT Formula

**rAthena pre-renewal source (status.cpp):**

```
Player HIT = BaseLv + DEX + BonusHIT
Monster HIT = BaseLv + DEX
```

**iRO Wiki / display formula (status window):**

```
Player HIT = 175 + BaseLv + DEX + BonusHIT
```

**Discrepancy note:** The rAthena source code for pre-renewal uses `level + status->dex` (no +175 base), while iRO Wiki Classic and the status window display include a +175 base. This discrepancy exists because the hit rate formula (`80 + AttackerHIT - DefenderFLEE`) effectively absorbs the constant. When both sides use the same base constants, the math produces the same accuracy percentage regardless:

- With +175 base: HIT = 175 + Lv + DEX, FLEE = 100 + Lv + AGI. HitRate = 80 + (175+Lv+DEX) - (100+Lv+AGI) = 155 + DEX - AGI + (attacker stats vs defender stats)
- Without base: HIT = Lv + DEX, FLEE = Lv + AGI. HitRate = 80 + (Lv+DEX) - (Lv+AGI) = 80 + DEX - AGI + ...

The key is consistency: as long as HIT and FLEE use the same base constant system, the accuracy calculation is correct. **Sabri_MMO uses the iRO Wiki formula** (175 + Lv + DEX for HIT, 100 + Lv + AGI for FLEE) which matches the status window display.

**BonusHIT sources:**
- Equipment cards (+HIT cards like Drops Card +1 HIT)
- Passive skills (Vulture's Eye, True Sight, etc.)
- Active buffs (Concentration, True Sight, Blessing via DEX)
- Skill-specific HIT bonuses (Bash +5% per level, Magnum Break +10% per level)

**Monster HIT:**
```
Monster HIT = Monster BaseLv + Monster DEX
```

Some sources cite 170 + Level + DEX for monster HIT, but this may conflate Renewal and pre-Renewal.

### 2.2 Accuracy Calculation (HIT vs FLEE)

```
HitRate% = 80 + AttackerHIT - DefenderFLEE
```

**Clamped to 5% minimum and 95% maximum.**

- **5% minimum:** Even with vastly inferior HIT, every attack has at least a 5% chance to land
- **95% maximum:** Even with vastly superior HIT, there is always a 5% chance to miss (via HIT/FLEE alone)
- **Perfect HIT (100%):** Only achievable through skills with `forceHit` flag, not through raw HIT stat

**To achieve 95% hit rate (cap):**
```
Required HIT >= DefenderFLEE + 15
```

**To achieve minimum 5% hit rate (floor):**
```
HIT <= DefenderFLEE - 75
```

**Practical example:**
- Player: Level 80, DEX 60, HIT bonus +10 -> HIT = 175 + 80 + 60 + 10 = 325
- Monster: FLEE 280
- HitRate = 80 + 325 - 280 = 125% -> capped at 95%

### 2.3 Perfect HIT Skills

Certain skills bypass the HIT/FLEE check entirely (100% hit rate):

| Skill | Mechanism |
|-------|-----------|
| Bash Lv5+ | +5% HIT rate per level (at Lv10: +50% multiplicative, plus Lv10 has `forceHit` in some implementations) |
| Magnum Break | +10% HIT rate per level (multiplicative bonus to hit rate) |
| Double Attack (Thief) | Always hits when it triggers (bypasses accuracy) |
| Bowling Bash | Force hit (always connects) |
| Pierce | Force hit |
| Brandish Spear | Force hit |
| Soul Breaker | Force hit (magic portion always hits; physical portion checks accuracy) |
| Acid Terror | Force hit (ignores FLEE) |

**Note:** "Force hit" means the skill sets `forceHit = true` in the damage calculation, which skips the HIT/FLEE check entirely. This is distinct from high HIT rate bonuses, which still respect the 95% cap.

### 2.4 Skill HIT Rate Bonuses

Some skills modify the hit rate multiplicatively before clamping:

```
Effective HitRate = floor(BaseHitRate * (100 + SkillHitRatePercent) / 100)
Clamped to 5-95%
```

| Skill | HIT Rate Bonus |
|-------|---------------|
| Bash | +5% per skill level (Lv10 = +50%) |
| Magnum Break | +10% per skill level (Lv10 = +100%) |

This means Bash Lv10 effectively doubles the hit rate before clamping. Against a monster where base hit rate would be 50%, Bash Lv10 raises it to 75%.

---

## 3. FLEE System

FLEE (evasion) determines the chance to dodge physical auto-attacks and some physical skills.

### 3.1 FLEE Formula

**rAthena pre-renewal source (status.cpp):**

```
Player FLEE = BaseLv + AGI + BonusFLEE
Monster FLEE = BaseLv + AGI
```

**iRO Wiki / display formula (status window):**

```
Player FLEE = 100 + BaseLv + AGI + BonusFLEE
```

The status window displays FLEE as "A + B" where:
- **A** = 100 + BaseLv + AGI + EquipmentFLEE (the main FLEE value)
- **B** = Perfect Dodge value (see Section 3.3)

**BonusFLEE sources:**
- Equipment cards (+FLEE cards like Whisper Card +20 FLEE)
- Passive skills (Improve Dodge: +1 FLEE per level, up to +10)
- Active buffs (Increase AGI adds AGI which increases FLEE indirectly, Wind Walk, etc.)
- Spear Quicken (+2 FLEE per skill level)

### 3.2 Dodge Rate Calculation

From the defender's perspective:

```
DodgeRate% = 100% - (80 + AttackerHIT - DefenderFLEE)
           = 20% - AttackerHIT + DefenderFLEE
```

Or equivalently:
```
DodgeRate% = 100% - HitRate%
```

**Capped at 0% minimum and 95% maximum** (mirrors the HIT rate floor/cap).

**What FLEE does not work against:**
- Magic attacks (always hit, bypass FLEE entirely)
- Skills with `forceHit` flag
- Critical hits (bypass the HIT/FLEE check)
- Ground-targeted AoE skills (no accuracy check)
- Traps

### 3.3 Perfect Dodge (Lucky Dodge)

Perfect Dodge (PD) is a **separate, independent** evasion check that occurs **before** the normal HIT/FLEE accuracy roll.

```
Perfect Dodge% = 1 + floor(LUK / 10) + BonusPD
```

**Key properties:**
- Each point of PD = 1% chance to completely avoid the attack
- **No cap** -- 100% Perfect Dodge is theoretically achievable with enough LUK + equipment
- Checked **before** HIT/FLEE -- if PD triggers, the attack misses regardless of HIT
- **Not reduced** by the multi-attacker FLEE penalty (PD is independent of FLEE)
- **Does NOT work against:** skills, traps, or magic -- only auto-attacks
- **Players only** -- monsters do not have Perfect Dodge (homunculus FLEE works similarly but is a separate system)

**Perfect Dodge vs Critical:**
- In the iRO Wiki Classic source: "This overrides (potential) critical hits" -- PD takes precedence over crits
- **However**, most rAthena implementations and the commonly accepted mechanic is: **Critical hits bypass Perfect Dodge**
- The check order in rAthena is: Critical check first -> if not crit, PD check -> if not PD, FLEE check
- **Sabri_MMO implementation:** Critical bypasses PD (aligns with rAthena standard)

**BonusPD sources:**

| Source | PD Bonus |
|--------|----------|
| Yoyo Card | +5 |
| Choco Card | +5 |
| Wild Rose Card | +5 (Thief class only) |
| Fortune Sword [Dagger] | +20 (also +5 LUK) |
| Morrigane's Manteau | +8 (also +2 LUK) |
| Valkyrian Manteau | +5 + (refine * 2) (Mage/Archer/Acolyte class only) |
| A Whistle (Bard song) | +(skill_level + 1) PD to party |

### 3.4 FLEE Penalty (Multiple Attackers)

When a character is attacked by more than 2 monsters simultaneously, FLEE is reduced:

```
Effective FLEE = SkillBonusFLEE + floor((BaseLv + AGI + EquipFLEE) * (1 - (NumAttackers - 2) * 0.10))
```

**Critical detail:** Skill-granted FLEE (like Improve Dodge) is added **after** the penalty calculation, not before. This protects skill investments from the mob penalty.

| Attackers | FLEE Remaining | Penalty |
|-----------|---------------|---------|
| 1         | 100%          | 0%      |
| 2         | 100%          | 0%      |
| 3         | 90%           | 10%     |
| 4         | 80%           | 20%     |
| 5         | 70%           | 30%     |
| 6         | 60%           | 40%     |
| 7         | 50%           | 50%     |
| 8         | 40%           | 60%     |
| 9         | 30%           | 70%     |
| 10        | 20%           | 80%     |
| 11        | 10%           | 90%     |
| 12+       | 0%            | 100%    |

**At 12+ simultaneous attackers**, effective FLEE drops to 0 and every attack lands (though Perfect Dodge still works independently).

**Counting attackers:** An "attacker" is any monster currently targeting the player for auto-attacks. Monsters that are merely aggroed but not actively swinging are typically not counted.

**Sabri_MMO implementation note:** Our current implementation uses a simpler flat penalty (`effectiveFlee = targetFlee - (numAttackers - 2) * 10`) rather than the percentage-based formula. This is a known discrepancy -- see Gap Analysis.

### 3.5 Boss Monsters and FLEE

Boss-type monsters (MVPs and mini-bosses) do **not** have special FLEE bypass mechanics in pre-Renewal. They use the same HIT/FLEE system as normal monsters. However:

- MVPs typically have very high HIT stats (high base level + high DEX), making them difficult to dodge
- Players can still technically dodge MVP attacks with enough FLEE
- Some MVP skills use `forceHit` which bypasses FLEE entirely
- The multi-attacker penalty combined with MVP slave monsters makes pure FLEE builds impractical against MVPs

---

## 4. Critical System

Critical hits bypass FLEE and deal bonus damage. They are the primary damage mechanism for LUK-based builds.

### 4.1 Critical Rate Formula

```
StatusCritical = 1 + floor(LUK * 0.3)
EquipCritical  = sum of all +CRIT equipment bonuses
TotalCritical  = StatusCritical + EquipCritical
```

**Status window display:** `floor(LUK / 3) + EquipBonuses` (slightly different rounding than the formula above)

**CRI per LUK point:** Each 1 LUK adds 0.3% critical rate. Every 10 LUK adds 3% crit.

| LUK | StatusCritical |
|-----|---------------|
| 1   | 1             |
| 10  | 4             |
| 20  | 7             |
| 30  | 10            |
| 40  | 13            |
| 50  | 16            |
| 60  | 19            |
| 70  | 22            |
| 80  | 25            |
| 90  | 28            |
| 99  | 30            |

**Common CRI equipment bonuses:**
- Soldier Skeleton Card: +9 CRI
- Kobold Card (Weapon): +4 CRI (Lv3 weapon)
- Crit Ring: +5 CRI
- Fortune Sword: +20 Lucky (contributes via LUK, not direct CRI)

### 4.2 Target's Critical Shield

The target's LUK reduces the attacker's effective critical rate:

```
CritShield = floor(TargetLUK / 5)
EffectiveCritRate = max(0, TotalCritical - CritShield)
```

Each 5 points of target LUK reduces incoming critical chance by 1%.

| Target LUK | CritShield |
|------------|-----------|
| 1-4        | 0         |
| 5-9        | 1         |
| 10-14      | 2         |
| 15-19      | 3         |
| 20-24      | 4         |
| 25-29      | 5         |
| 50         | 10        |
| 77         | 15        |
| 99         | 19        |

**Example:** Player with 30 CRI attacking a monster with 20 LUK:
```
CritShield = floor(20 / 5) = 4
EffectiveCrit = 30 - 4 = 26%
```

**Note:** Some sources use `floor(TargetLUK * 0.2)` which gives the same result as `floor(TargetLUK / 5)`.

### 4.3 Katar Critical Bonus

Katar-class weapons **double** the total critical rate:

```
KatarCritical = TotalCritical * 2
```

This doubling occurs **after** adding equipment bonuses but **before** subtracting the target's CritShield:

```
EffectiveCrit = (StatusCritical + EquipCritical) * 2 - CritShield
```

This makes Assassins with Katars the premier critical-build class. A 90 LUK Assassin with Katar:
```
StatusCrit = 1 + floor(90 * 0.3) = 28
KatarDouble = 28 * 2 = 56%
After CritShield (enemy LUK 20, shield=4): 56 - 4 = 52% crit rate
```

The doubling is **not shown** in the status window -- it is applied internally during combat calculations only.

### 4.4 Critical Damage Calculation

Critical hits modify the normal damage pipeline in several ways:

#### 4.4.1 What Criticals Change

1. **Maximum WeaponATK:** No variance roll -- always use the highest possible weapon damage
2. **Bypass FLEE:** The HIT/FLEE accuracy check is skipped entirely (criticals always hit)
3. **40% damage bonus:** Final damage is multiplied by 1.4x
4. **DEF bypass (pre-Renewal):** In pre-Renewal, critical hits **ignore both Hard DEF and Soft DEF** entirely

```
CriticalDamage = floor((StatusATK + MaxWeaponATK + PassiveATK) * ElementModifier / 100 * 1.4)
```

#### 4.4.2 DEF Bypass Detail

This is one of the most contested mechanics across sources:

- **iRO Wiki Classic:** "Critical Hit rating does damage that fully ignores enemy DEF, both the percentage and pure value part" -- criticals bypass both Hard DEF and Soft DEF
- **rAthena pre-renewal default:** Critical hits bypass both Hard DEF and Soft DEF (`battle_config.critical_def` defaults to 0, meaning 0% DEF applied to crits)
- **Renewal change:** In Renewal, criticals no longer bypass DEF (only bypass FLEE + add 40% damage)

**For pre-Renewal implementation:** Criticals should bypass both Hard DEF (equipment %) and Soft DEF (VIT flat) entirely. The full critical damage formula is:

```
1. StatusATK = STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
2. WeaponATK = MaxWeaponATK (no variance)
3. SizedWeaponATK = floor(WeaponATK * SizePenalty / 100)
4. BaseDamage = StatusATK + SizedWeaponATK + MasteryATK
5. After card bonuses = floor(BaseDamage * (100 + CardBonus) / 100)
6. After element = floor(BaseDamage * ElementModifier / 100)
7. Skip Hard DEF (bypass)
8. Skip Soft DEF (bypass)
9. CriticalDamage = floor(BaseDamage * 1.4)
10. Floor to minimum 1
```

#### 4.4.3 Critical Damage vs Perfect Dodge

**Standard rAthena behavior:** Critical hits **bypass** Perfect Dodge.

The check order is:
```
1. Is it a critical hit? -> YES: Skip PD check AND FLEE check. Attack always lands.
2. Not a critical -> Check Perfect Dodge (LUK/10 + bonuses)% chance to evade
3. Not PD'd -> Check FLEE (80 + HIT - FLEE)% chance to hit
```

Some iRO Wiki sources state PD "overrides critical hits," meaning PD is checked first and can prevent a crit. This is a known discrepancy between sources. **The rAthena standard (crits bypass PD) is the widely accepted implementation.**

### 4.5 Skills and Critical Hits

**In pre-Renewal, skills cannot critically hit.** Critical hits apply only to auto-attacks (normal attacks). This means:

- Bash, Bowling Bash, Pierce, Brandish Spear, etc. **cannot** crit
- Double Attack (Thief passive) is **not** a crit -- it triggers a separate double-hit mechanic
- Triple Attack (Monk passive) is **not** a crit
- Sharp Shooting (Hunter/Sniper) deals "fake critical" damage -- it ignores DEF/FLEE like a crit but does not actually trigger critical hit mechanics
- Auto Counter (Knight) when in active counter mode applies a 2x crit rate bonus but still rolls normally

**The only exception** in pre-Renewal for skill crits is certain interaction scenarios where auto-attacks trigger during skill effects (e.g., Hindsight/Auto Spell procs are separate hits, not skill crits).

### 4.6 Critical Hit Rate for Monsters

Monsters can also deal critical hits:
```
Monster CritRate = 1 + floor(MonsterLUK / 3)
```

Some monsters have specific critical hit skills (Critical Slash, etc.) that force crits.

### 4.7 Anti-Critical Items and Effects

| Source | Effect |
|--------|--------|
| Target's LUK | CritShield = floor(LUK/5) reduces incoming crit rate |
| Steel Body (Monk) | Immune to critical hits while active |
| Cranial Buckler (Shield) | No direct anti-crit, but reduced damage from Demi-Human |

Anti-critical defense is primarily stat-based (LUK) rather than equipment-based in pre-Renewal.

---

## 5. Edge Cases

### 5.1 Frozen/Stone Status and ASPD

- **Frozen:** ASPD drops to 0 (cannot attack). Also sets DEF element to Water Lv1.
- **Stoned (full petrification):** ASPD drops to 0. Also sets DEF element to Earth Lv1.
- **Stun:** Cannot attack (not ASPD reduction -- complete action lock).
- **Sleep:** Cannot attack. Broken by taking damage.

### 5.2 ASPD Under Decrease AGI

Decrease AGI reduces the AGI stat, which in turn reduces ASPD through the formula. It does not directly modify ASPD -- it works through stat modification.

### 5.3 Skills That Ignore FLEE

The following skills bypass the HIT/FLEE check entirely:

- All magic skills (Fire Bolt, Storm Gust, etc.)
- Skills with `forceHit` flag (Bowling Bash, Pierce, Acid Terror, etc.)
- Ground-targeted AoE skills
- Traps (Ankle Snare, Land Mine, etc.)
- Auto-attacks after Double Attack triggers

### 5.4 Attack Speed Stacking Rules

1. **Potions:** Only the highest potion applies (Concentration < Awakening < Berserk). They do NOT stack with each other.
2. **Skill buffs:** Skills from the same exclusion group do not stack (THQ/OHQ/SQ/AR -- strongest wins).
3. **Potions + Skills:** These two categories DO stack additively.
4. **Equipment ASPD:** Stacks additively with everything.
5. **AGI buffs:** Increase AGI does NOT provide a speed modifier -- it adds AGI stat points which flow through the formula normally.

### 5.5 FLEE in WoE (War of Emperium)

During WoE siege, FLEE dodge rate has no cap (can exceed 95%). However, the multi-attacker penalty still applies, and the sheer volume of attackers in WoE typically renders FLEE builds impractical.

### 5.6 Status Window Display vs Internal Values

The status window displays HIT, FLEE (as A+B), and CRI with rounded/formatted values. Internal calculations may differ:

| Stat | Status Window | Internal |
|------|--------------|----------|
| HIT | 175 + Lv + DEX + bonuses | Same or Lv + DEX (depends on base constant system) |
| FLEE | (100 + Lv + AGI + bonuses) + PerfectDodge | Same, PD shown as second number |
| CRI | floor(LUK/3) + equip bonuses | 1 + floor(LUK * 0.3) + bonuses (katar x2 not shown) |
| ASPD | Displayed as 0-190 integer | Internal may use higher precision |

### 5.7 Weapon Swap and ASPD Recalculation

Changing weapons triggers an immediate ASPD recalculation. This is relevant for:
- Two-Hand Quicken: Swapping from 2H Sword to any other weapon cancels the buff
- Adrenaline Rush: Swapping from Axe/Mace to another weapon type cancels the buff
- Katar CRI bonus: Only active while katar is equipped

### 5.8 Dual Attack Timing (Dual Wield)

When dual-wielding, both hands attack each combat tick cycle:
1. Right hand damage is calculated with full right-hand weapon + cards
2. Left hand damage is calculated with left-hand weapon + cards
3. Left hand has a mastery penalty (reduced damage) based on Righthand Mastery / Lefthand Mastery skill levels
4. Both hits share the same attack interval (determined by dual-wield ASPD)
5. Both hits are sent as separate damage events to the client

---

## 6. Implementation Checklist

### ASPD System

- [x] Core ASPD formula: `200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SM)`
- [x] Base ASPD table per class and weapon type (`ASPD_BASE_DELAYS` in `ro_exp_tables.js`)
- [x] Dual wield ASPD: `WD_dual = floor((WD_right + WD_left) * 0.7)`
- [x] Speed modifier from skill buffs (THQ, AR, OHQ, SQ, Frenzy)
- [x] ASPD potion reduction (separate from formula SM, applied to final interval)
- [x] ASPD cap at 190 (with optional soft cap to 195 in our implementation)
- [x] Attack delay conversion: `(200 - ASPD) * 20` ms base (with our custom formula above 190)
- [x] Mount penalty: 50% base, +10% per Cavalry Mastery level
- [x] Transcendent class -> base class mapping for ASPD table lookup
- [x] Weapon swap ASPD recalculation (THQ/AR cancellation on swap)
- [ ] Shield penalty (flat ASPD reduction when shield equipped) -- partially implemented
- [ ] Equipment ASPD rate bonuses (Doppelganger Card, etc.) -- verify stacking

### HIT System

- [x] HIT formula: `175 + BaseLv + DEX + BonusHIT`
- [x] Hit rate: `80 + AttackerHIT - DefenderFLEE`, clamped 5-95%
- [x] Skill HIT rate bonuses (Bash, Magnum Break multiplicative bonus)
- [x] Force hit for specific skills (Pierce, Bowling Bash, Acid Terror, etc.)
- [x] Monster HIT: `BaseLv + DEX` (used for monster-vs-player accuracy)
- [ ] Verify monster HIT base constant (170 vs none vs 175)

### FLEE System

- [x] FLEE formula: `100 + BaseLv + AGI + BonusFLEE`
- [x] Multi-attacker FLEE penalty
- [x] Perfect Dodge: `1 + floor(LUK/10) + BonusPD`
- [x] PD only works against auto-attacks (not skills/magic)
- [x] Critical bypasses PD
- [ ] FLEE penalty uses flat subtraction instead of percentage-based reduction (see Gap Analysis)
- [ ] Skill FLEE bonus added after penalty (not before)

### Critical System

- [x] Critical rate: `1 + floor(LUK * 0.3) + EquipCRI`
- [x] Katar double critical: `totalCrit * 2`
- [x] Critical shield: `floor(TargetLUK / 5)` -- implemented as `floor(TargetLUK * 0.2)`
- [x] Critical damage: 1.4x multiplier
- [x] Critical bypasses FLEE check
- [x] Skills cannot crit in pre-Renewal
- [x] Critical uses maximum weapon ATK (no variance)
- [ ] Verify critical bypasses both Hard DEF and Soft DEF (currently applies both)

---

## 7. Gap Analysis

Comparison between canonical pre-Renewal RO mechanics and the current Sabri_MMO implementation.

### 7.1 ASPD Base Delay Values Mismatch

**Issue:** The `ASPD_BASE_DELAYS` table in `ro_exp_tables.js` uses values that do not match rAthena's `job_aspd.yml`.

**rAthena values** are in centiseconds (e.g., Knight Fist = 400, Knight 2H Sword = 550).

**Our values** appear to be hand-tuned approximations (e.g., Knight bare_hand = 38, Knight two_hand_sword = 50).

**Impact:** Different classes may have incorrect relative attack speeds. For example:
- rAthena Knight 2H Sword: 550 vs our 50 (we use ~1/10 scale, but ratios differ)
- rAthena Monk Knuckle: 475 vs our 42 (475/10 = 47.5, we have 42)
- rAthena Assassin Katar: 500 vs our 42 (500/10 = 50, we have 42)

**Recommendation:** Align to rAthena values divided by 10 (converting centiseconds to the WD scale used in the formula). Alternatively, adopt the rAthena values directly and adjust the formula to use centisecond-based calculations.

### 7.2 FLEE Penalty Formula (Flat vs Percentage)

**Issue:** Our `calculateHitRate()` uses a flat penalty: `effectiveFlee = targetFlee - (numAttackers - 2) * 10`

**Canonical formula:** `effectiveFlee = floor(baseFlee * (1 - (numAttackers - 2) * 0.10)) + skillFlee`

**Impact:** With flat penalty, a character with 300 FLEE loses 10 FLEE per extra attacker. With percentage penalty, they lose 30 FLEE per extra attacker. The flat penalty is much more lenient for high-FLEE characters.

**Recommendation:** Switch to percentage-based penalty and ensure skill FLEE is added after the penalty calculation.

### 7.3 Critical DEF Bypass

**Issue:** Our `calculatePhysicalDamage()` applies Hard DEF and Soft DEF to critical hits the same as normal hits, then multiplies by 1.4x.

**Canonical behavior:** Pre-renewal critical hits bypass both Hard DEF and Soft DEF entirely. Damage is calculated without any DEF reduction, then multiplied by 1.4x.

**Impact:** Critical damage is significantly lower than it should be. A crit that should deal 1400 damage (before DEF) currently deals `floor((1400 * (1 - hardDEF/100) - softDEF) * 1.4)` instead of `floor(1400 * 1.4)`.

**Recommendation:** Add a check in the damage pipeline: if `isCritical`, skip steps 13 (Hard DEF) and 14 (Soft DEF) before applying the 1.4x multiplier.

### 7.4 Attack Delay Formula

**Issue:** Our `getAttackIntervalMs()` uses `(200 - aspd) * 50` for ASPD <= 195, while the canonical formula is `(200 - ASPD) * 20`.

**Impact:** At ASPD 170: our formula gives 1500ms, canonical gives 600ms. Our characters attack 2.5x slower than they should at all ASPD levels.

**Wait -- re-reading the code:** `COMBAT.ASPD_CAP` may be set differently. Need to verify what value `ASPD_CAP` has and whether the `* 50` is intentional tuning or a bug. The canonical pre-renewal formula gives `(200 - ASPD) / 50` in seconds, which is `(200 - ASPD) * 20` in milliseconds.

**Recommendation:** Verify `ASPD_CAP` value and whether `* 50` is intentional. If not, correct to `* 20`.

### 7.5 Monster FLEE and HIT Base Constants

**Issue:** Need to verify whether monster HIT uses `BaseLv + DEX` (rAthena pre-renewal) or `170 + BaseLv + DEX` (some sources cite for monsters).

**Impact:** If monsters use a different base constant than players, accuracy calculations would be asymmetric.

**Recommendation:** Check rAthena `status.cpp` for monster-specific HIT/FLEE formulas and align.

### 7.6 ASPD Potion Application

**Issue:** Our implementation applies ASPD potion reduction as a percentage reduction to the final interval (`interval * (1 - reduction)`), separate from the SM formula. The canonical approach is to include potion SM in the main ASPD formula (`(1 - SM)` where SM includes potion value).

**Impact:** These produce different results. The current approach is simpler but may yield slightly different ASPD values at the margins.

**Recommendation:** Move potion SM into the main ASPD formula for accuracy, or verify that the current approach produces acceptable approximations.

### 7.7 Missing Weapon Types in ASPD Table

**Issue:** Several weapon types present in rAthena are missing from our `ASPD_BASE_DELAYS`:
- `2hMace` (Swordsman, Acolyte, Merchant, Knight, Crusader, Priest, Monk, Blacksmith, Alchemist)
- `2hAxe` (Swordsman, Merchant, Knight, Crusader, Blacksmith, Alchemist)
- `2hStaff` (Novice, Mage, Acolyte, Wizard, Priest, Monk, Sage)
- `book` (Priest, Sage)
- Some entries use combined `spear` key instead of separate `1hSpear`/`2hSpear`
- Some entries use combined `axe` key instead of separate `1hAxe`/`2hAxe`

**Impact:** Characters equipping these weapon subtypes may fall back to `bare_hand` delay, producing incorrect ASPD.

**Recommendation:** Expand the ASPD table to include all rAthena weapon subtypes, or add fallback logic (e.g., `2hAxe` falls back to `1hAxe` delay if not found).

---

## Sources

- [ASPD - iRO Wiki Classic](https://irowiki.org/classic/ASPD)
- [FLEE - iRO Wiki Classic](https://irowiki.org/classic/FLEE)
- [Perfect Dodge - iRO Wiki Classic](https://irowiki.org/classic/Perfect_Dodge)
- [Stats - iRO Wiki Classic](https://irowiki.org/classic/Stats)
- [Stats (RO) - Ragnarok Wiki Fandom](https://ragnarok.fandom.com/wiki/Stats_(RO))
- [rAthena status.cpp (pre-renewal ASPD/HIT/FLEE)](https://github.com/rathena/rathena/blob/master/src/map/status.cpp)
- [rAthena battle.cpp (hit rate calculation)](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
- [rAthena db/pre-re/job_aspd.yml (BaseASPD table)](https://github.com/rathena/rathena/blob/master/db/pre-re/job_aspd.yml)
- [Hit and Flee in Pre-Renewal - rAthena Forum](https://rathena.org/board/topic/133367-hit-and-flee-in-pre-renewal/)
- [ASPD Potions - iRO Wiki](https://irowiki.org/wiki/ASPD_Potion)
- [Critical Hit Rate - TalonRO Wiki](https://wiki.talonro.com/Critical_Hit_Rate)
- [RagnaPlace ASPD Mirror](https://ragnaplace.com/en/wiki/irowiki-classic/ASPD)
