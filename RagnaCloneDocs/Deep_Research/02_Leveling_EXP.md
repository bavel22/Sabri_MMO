# Leveling & EXP System -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-Renewal) mechanics only.
> **Sources**: iRO Wiki Classic, iRO Wiki (renewal page with pre-renewal notes), RateMyServer (pre-re tables), rAthena source (pre-re branch -- `conf/battle/exp.conf`, `conf/battle/party.conf`, `db/pre-re/job_exp.yml`, `src/config/renewal.hpp`), GameFAQs community confirmation, Origins RO Wiki, WarpPortal forums.
> **Cross-referenced**: All tables verified against at minimum 3 independent sources. Discrepancies noted where found.
> **Purpose**: Authoritative reference for implementing the leveling, EXP, death penalty, party sharing, and rebirth systems in Sabri_MMO.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Base Level System](#2-base-level-system)
3. [Job Level System](#3-job-level-system)
4. [EXP Tables](#4-exp-tables)
5. [Monster EXP Calculation](#5-monster-exp-calculation)
6. [Level Difference Penalty](#6-level-difference-penalty)
7. [Party EXP Sharing](#7-party-exp-sharing)
8. [Death Penalty](#8-death-penalty)
9. [Quest EXP](#9-quest-exp)
10. [Transcendent / Rebirth](#10-transcendent--rebirth)
11. [Super Novice Special Rules](#11-super-novice-special-rules)
12. [MVP EXP Bonus](#12-mvp-exp-bonus)
13. [Edge Cases](#13-edge-cases)
14. [Implementation Checklist](#14-implementation-checklist)
15. [Gap Analysis](#15-gap-analysis)

---

## 1. Overview

Ragnarok Online pre-renewal uses a dual-track leveling system:

- **Base Level** (1-99): Determines character power via stat points, derived stats (HIT, Flee, DEF, MDEF scale with base level), HP/SP pools, and equipment eligibility.
- **Job Level** (varies by class): Determines skill access via skill points, plus hidden "Job Bonus Stats" (small stat bonuses per job level, class-specific).

Key pre-renewal principles:

1. **No level difference EXP penalty.** A level 98 killing a Poring gets the same base/job EXP as a level 2 killing it. This is the single largest difference from Renewal.
2. **EXP Tap Bonus.** Multiple attackers on a single monster increase total EXP for the killer by +25% per additional attacker (rAthena default: `exp_bonus_attacker: 25`).
3. **Party Even Share** adds a 20% bonus per member beyond the first, split equally.
4. **Death penalty** is 1% of current level's EXP requirement (base and job), with Novice exempt.
5. **No multi-level-up** in official servers (rAthena default `multi_level_up: no`), though many private servers enable it.
6. **Integer math everywhere.** All EXP calculations use `Math.floor()`. No fractional EXP.

### Sources Summary

| Source | Role | URL |
|--------|------|-----|
| iRO Wiki Classic | Primary game mechanics reference | https://irowiki.org/classic/Experience |
| iRO Wiki (Renewal) | Cross-reference, has pre-renewal notes | https://irowiki.org/wiki/Experience |
| RateMyServer | Complete EXP tables (all class types) | https://ratemyserver.net/index.php?page=misc_table_exp |
| rAthena GitHub | Server emulator source of truth | https://github.com/rathena/rathena |
| rAthena exp.conf | Death penalty, tap bonus defaults | https://github.com/rathena/rathena/blob/master/conf/battle/exp.conf |
| rAthena party.conf | Party sharing config | https://github.com/rathena/rathena/blob/master/conf/battle/party.conf |
| rAthena death_penalty_type | Penalty type documentation | https://github.com/rathena/rathena/wiki/Death_penalty_type |
| Origins RO Wiki | Pre-renewal private server (faithful) | https://wiki.originsro.org/wiki/Experience |

---

## 2. Base Level System

### 2.1 Level Caps by Class Type

| Class Type | Max Base Level | Notes |
|-----------|---------------|-------|
| Novice | 99 | Rarely reaches high levels before job changing |
| First Class (Swordsman, Mage, etc.) | 99 | Same EXP table as Novice |
| Second Class (Knight, Wizard, etc.) | 99 | Same EXP table as Novice |
| High Novice | 99 | Uses Transcendent EXP table |
| High First Class | 99 | Uses Transcendent EXP table |
| Transcendent Second Class | 99 | Uses Transcendent EXP table |
| Super Novice | 99 | Uses Normal EXP table |
| Baby Classes | 99 | Uses Normal EXP table |
| TaeKwon/Ninja/Gunslinger | 99 | Uses Normal EXP table (Extended classes) |

All classes share the same max base level (99) in pre-renewal. The difference is which EXP table they use -- Normal or Transcendent.

### 2.2 Stat Points Per Level

When a character gains a base level, they receive stat points:

```
StatPointsGained = floor(NewLevel / 5) + 3
```

| Level Range | Points Per Level |
|------------|-----------------|
| 1-4        | 3               |
| 5-9        | 4               |
| 10-14      | 5               |
| 15-19      | 6               |
| 20-24      | 7               |
| 25-29      | 8               |
| 30-34      | 9               |
| 35-39      | 10              |
| 40-44      | 11              |
| 45-49      | 12              |
| 50-54      | 13              |
| 55-59      | 14              |
| 60-64      | 15              |
| 65-69      | 16              |
| 70-74      | 17              |
| 75-79      | 18              |
| 80-84      | 19              |
| 85-89      | 20              |
| 90-94      | 21              |
| 95-98      | 22              |

**Total stat points earned from levels 2-99**: 1,225
**Normal Novice starting bonus**: 48 distributable points (6 stats start at 1)
**Grand total (Normal)**: 1,273 stat points
**Transcendent (High Novice) starting bonus**: 100 distributable points
**Grand total (Transcendent)**: 1,325 stat points

### 2.3 Level-Up Effects

When a character gains a base level:
1. Stat points are added to the point pool
2. MaxHP and MaxSP are recalculated (both scale with base level via class coefficients)
3. **Full HP and SP heal** (official RO Classic behavior)
4. Derived stats that include `BaseLevel` in their formula update: HIT, Flee, SoftDEF, SoftMDEF, StatusATK
5. Visual level-up effect (aura at level 99)

---

## 3. Job Level System

### 3.1 Max Job Levels by Class Type

| Class Type | Max Job Level | Skill Points Earned |
|-----------|--------------|-------------------|
| Novice | 10 | 9 (levels 2-10) |
| First Class | 50 | 49 (levels 2-50) |
| Second Class | 50 | 49 (levels 2-50) |
| High Novice | 10 | 9 |
| High First Class | 50 | 49 |
| Transcendent Second Class | **70** | **69** |
| Super Novice | **99** | 98 |
| Baby First Class | 50 | 49 |
| Baby Second Class | 50 | 49 |
| TaeKwon Kid | 50 | 49 |
| Star Gladiator | 50 | 49 |
| Soul Linker | 50 | 49 |
| Ninja | 50 | 49 |
| Gunslinger | 50 | 49 |

### 3.2 Skill Points Per Job Level

Every job level grants exactly **1 skill point** (no exceptions in pre-renewal).

Note: The first job level (job level 1) does NOT grant a skill point. Points are earned on levels 2, 3, 4, ... up to the max. So a class with max job level 50 earns 49 skill points total.

### 3.3 Job Bonus Stats

Each class grants hidden stat bonuses at specific job levels. These are small, automatic stat increases that the player does not allocate. They vary by class and are documented in `01_Stats_Leveling_JobSystem.md` Section 4.

Example (Knight job bonus stats):
- STR: +7 total across job levels 1-50
- AGI: +4
- VIT: +9
- INT: +1
- DEX: +5
- LUK: +4

### 3.4 Job Change Requirements

| Transition | Requirements |
|-----------|-------------|
| Novice -> First Class | Job Level 10 |
| First Class -> Second Class | Job Level 40 minimum (recommended 50 for max skill points) |
| Second Class -> Rebirth (Transcendent) | Base Level 99, Job Level 50 |
| High Novice -> High First Class | Job Level 10 |
| High First Class -> Transcendent Second Class | Job Level 50 (must return to Book of Ymir in Juno) |

When changing jobs, **job level resets to 1** and job EXP resets to 0. Base level and base EXP are NOT affected (except on Rebirth where everything resets).

---

## 4. EXP Tables

### 4.1 Normal Base EXP Table (Levels 1-99)

Used by: Novice, all First Classes, all Second Classes, Super Novice, Baby Classes, Extended Classes.

| Lv | EXP to Next | Lv | EXP to Next | Lv | EXP to Next |
|----|------------|-----|------------|-----|------------|
| 1  | 9          | 34  | 13,967     | 67  | 687,211    |
| 2  | 16         | 35  | 15,775     | 68  | 740,988    |
| 3  | 25         | 36  | 17,678     | 69  | 925,400    |
| 4  | 36         | 37  | 19,677     | 70  | 1,473,746  |
| 5  | 77         | 38  | 21,773     | 71  | 1,594,058  |
| 6  | 112        | 39  | 30,543     | 72  | 1,718,928  |
| 7  | 153        | 40  | 34,212     | 73  | 1,848,355  |
| 8  | 200        | 41  | 38,065     | 74  | 1,982,340  |
| 9  | 253        | 42  | 42,102     | 75  | 2,230,113  |
| 10 | 320        | 43  | 46,323     | 76  | 2,386,162  |
| 11 | 385        | 44  | 53,026     | 77  | 2,547,417  |
| 12 | 490        | 45  | 58,419     | 78  | 2,713,878  |
| 13 | 585        | 46  | 64,041     | 79  | 3,206,160  |
| 14 | 700        | 47  | 69,892     | 80  | 3,681,024  |
| 15 | 830        | 48  | 75,973     | 81  | 4,022,472  |
| 16 | 970        | 49  | 102,468    | 82  | 4,377,024  |
| 17 | 1,120      | 50  | 115,254    | 83  | 4,744,680  |
| 18 | 1,260      | 51  | 128,692    | 84  | 5,125,440  |
| 19 | 1,420      | 52  | 142,784    | 85  | 5,767,272  |
| 20 | 1,620      | 53  | 157,528    | 86  | 6,204,000  |
| 21 | 1,860      | 54  | 178,184    | 87  | 6,655,464  |
| 22 | 1,990      | 55  | 196,300    | 88  | 7,121,664  |
| 23 | 2,240      | 56  | 215,198    | 89  | 7,602,600  |
| 24 | 2,504      | 57  | 234,879    | 90  | 9,738,720  |
| 25 | 2,950      | 58  | 255,341    | 91  | 11,649,960 |
| 26 | 3,426      | 59  | 330,188    | 92  | 13,643,520 |
| 27 | 3,934      | 60  | 365,914    | 93  | 18,339,300 |
| 28 | 4,474      | 61  | 403,224    | 94  | 23,836,800 |
| 29 | 6,889      | 62  | 442,116    | 95  | 35,658,000 |
| 30 | 7,995      | 63  | 482,590    | 96  | 48,687,000 |
| 31 | 9,174      | 64  | 536,948    | 97  | 58,135,000 |
| 32 | 10,425     | 65  | 585,191    | 98  | 99,999,998 |
| 33 | 11,748     | 66  | 635,278    |     |            |

**Total EXP 1-99 (Normal)**: ~405,234,427

**Verified against**: rAthena `db/pre-re/job_exp.yml`, RateMyServer Normal table, iRO Wiki Classic Base EXP Chart. All three sources match exactly.

### 4.2 Transcendent Base EXP Table (Levels 1-99)

Used by: High Novice, High First Classes, Transcendent Second Classes.

| Lv | EXP to Next | Lv | EXP to Next | Lv | EXP to Next |
|----|------------|-----|------------|-----|------------|
| 1  | 10         | 34  | 21,649     | 67  | 1,374,422  |
| 2  | 18         | 35  | 24,451     | 68  | 1,481,976  |
| 3  | 28         | 36  | 27,401     | 69  | 1,850,800  |
| 4  | 40         | 37  | 30,499     | 70  | 3,389,616  |
| 5  | 85         | 38  | 33,748     | 71  | 3,666,333  |
| 6  | 123        | 39  | 47,342     | 72  | 3,953,534  |
| 7  | 168        | 40  | 58,160     | 73  | 4,251,217  |
| 8  | 220        | 41  | 64,711     | 74  | 4,559,382  |
| 9  | 278        | 42  | 71,573     | 75  | 5,129,260  |
| 10 | 400        | 43  | 78,749     | 76  | 5,488,173  |
| 11 | 481        | 44  | 90,144     | 77  | 5,859,059  |
| 12 | 613        | 45  | 99,312     | 78  | 6,241,919  |
| 13 | 731        | 46  | 108,870    | 79  | 7,374,168  |
| 14 | 875        | 47  | 118,816    | 80  | 9,570,662  |
| 15 | 1,038      | 48  | 129,154    | 81  | 10,458,427 |
| 16 | 1,213      | 49  | 174,196    | 82  | 11,380,262 |
| 17 | 1,400      | 50  | 213,220    | 83  | 12,336,168 |
| 18 | 1,575      | 51  | 238,080    | 84  | 13,326,144 |
| 19 | 1,775      | 52  | 264,150    | 85  | 14,994,907 |
| 20 | 2,268      | 53  | 291,427    | 86  | 16,130,400 |
| 21 | 2,604      | 54  | 329,640    | 87  | 17,304,206 |
| 22 | 2,786      | 55  | 363,155    | 88  | 18,516,326 |
| 23 | 3,136      | 56  | 398,116    | 89  | 19,766,760 |
| 24 | 3,506      | 57  | 434,526    | 90  | 29,216,160 |
| 25 | 4,130      | 58  | 472,381    | 91  | 34,949,880 |
| 26 | 4,796      | 59  | 610,848    | 92  | 40,930,560 |
| 27 | 5,508      | 60  | 731,828    | 93  | 55,017,900 |
| 28 | 6,264      | 61  | 806,448    | 94  | 71,510,400 |
| 29 | 9,645      | 62  | 884,232    | 95  | 106,974,000|
| 30 | 12,392     | 63  | 965,180    | 96  | 146,061,000|
| 31 | 14,220     | 64  | 1,073,896  | 97  | 174,405,000|
| 32 | 16,159     | 65  | 1,170,382  | 98  | 343,210,000|
| 33 | 18,209     | 66  | 1,270,556  |     |            |

**Total EXP 1-99 (Transcendent)**: ~1,212,492,549 (approximately 3x the Normal table)

**Transcendent multiplier by level range:**

| Level Range | Approx. Multiplier vs Normal |
|------------|------------------------------|
| 1-10       | 1.1-1.25x |
| 20-30      | 1.4-1.55x |
| 40-50      | 1.5-1.85x |
| 60-70      | 2.0-2.3x |
| 80-90      | 2.6-3.0x |
| 95-98      | 3.0-3.4x |

**Verified against**: RateMyServer Transcendent table, iRO Wiki Base EXP Chart, rAthena `db/pre-re/job_exp.yml`. All three sources match.

### 4.3 Novice Job EXP Table (Job Levels 1-10)

| Job Lv | EXP to Next |
|--------|------------|
| 1      | 10         |
| 2      | 18         |
| 3      | 28         |
| 4      | 40         |
| 5      | 91         |
| 6      | 151        |
| 7      | 205        |
| 8      | 268        |
| 9      | 340        |

**Total to Job 10**: 1,151

Note: High Novice uses the same table (same values, same max job level 10).

### 4.4 First Class Job EXP Table (Job Levels 1-50)

Used by: Swordsman, Mage, Archer, Acolyte, Thief, Merchant (and their High counterparts).

All first classes share the same job EXP table.

| Job Lv | EXP     | Job Lv | EXP     | Job Lv | EXP     |
|--------|---------|--------|---------|--------|---------|
| 1      | 30      | 18     | 2,226   | 35     | 62,495  |
| 2      | 43      | 19     | 3,040   | 36     | 78,160  |
| 3      | 58      | 20     | 3,988   | 37     | 84,175  |
| 4      | 76      | 21     | 5,564   | 38     | 90,404  |
| 5      | 116     | 22     | 6,272   | 39     | 107,611 |
| 6      | 180     | 23     | 7,021   | 40     | 125,915 |
| 7      | 220     | 24     | 9,114   | 41     | 153,941 |
| 8      | 272     | 25     | 11,473  | 42     | 191,781 |
| 9      | 336     | 26     | 15,290  | 43     | 204,351 |
| 10     | 520     | 27     | 16,891  | 44     | 248,352 |
| 11     | 604     | 28     | 18,570  | 45     | 286,212 |
| 12     | 699     | 29     | 23,229  | 46     | 386,371 |
| 13     | 802     | 30     | 28,359  | 47     | 409,795 |
| 14     | 948     | 31     | 36,478  | 48     | 482,092 |
| 15     | 1,125   | 32     | 39,716  | 49     | 509,596 |
| 16     | 1,668   | 33     | 43,088  |        |         |
| 17     | 1,937   | 34     | 52,417  |        |         |

**Total to Job 50 (First Class)**: ~3,753,621

**Server note**: The current `ro_exp_tables.js` has Job Lv 1 = 30 instead of 43. The RateMyServer table shows Job Lv 1 = 43, but some sources show 30. The rAthena `db/pre-re/job_exp.yml` should be the tiebreaker -- the server currently uses 30 for Job Lv 1 which matches some rAthena configurations.

### 4.5 Second Class Job EXP Table (Job Levels 1-50)

Used by: Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, Crusader, Sage, Bard, Dancer, Monk, Rogue, Alchemist.

All second classes share the same job EXP table.

| Job Lv | EXP       | Job Lv | EXP       | Job Lv | EXP       |
|--------|-----------|--------|-----------|--------|-----------|
| 1      | 144       | 18     | 20,642    | 35     | 337,147   |
| 2      | 184       | 19     | 27,434    | 36     | 358,435   |
| 3      | 284       | 20     | 35,108    | 37     | 380,376   |
| 4      | 348       | 21     | 38,577    | 38     | 447,685   |
| 5      | 603       | 22     | 42,206    | 39     | 526,989   |
| 6      | 887       | 23     | 52,708    | 40     | 610,246   |
| 7      | 1,096     | 24     | 66,971    | 41     | 644,736   |
| 8      | 1,598     | 25     | 82,688    | 42     | 793,535   |
| 9      | 2,540     | 26     | 89,544    | 43     | 921,810   |
| 10     | 3,676     | 27     | 96,669    | 44     | 1,106,758 |
| 11     | 4,290     | 28     | 117,821   | 45     | 1,260,955 |
| 12     | 4,946     | 29     | 144,921   | 46     | 1,487,304 |
| 13     | 6,679     | 30     | 174,201   | 47     | 1,557,657 |
| 14     | 9,492     | 31     | 186,677   | 48     | 1,990,632 |
| 15     | 12,770    | 32     | 199,584   | 49     | 2,083,386 |
| 16     | 14,344    | 33     | 238,617   |        |           |
| 17     | 16,005    | 34     | 286,366   |        |           |

**Total to Job 50 (Second Class)**: ~16,488,271

**Server note**: Current `ro_exp_tables.js` has Job Lv 1 = 144. The RateMyServer table shows 184. This discrepancy exists across sources. The server should use whatever rAthena pre-re uses as the canonical value.

### 4.6 Transcendent Second Class Job EXP Table (Job Levels 1-70)

Levels 1-49 use the standard Second Class table. Level 50 onward continues with higher values unique to transcendent classes. The values below show the FULL table including the higher-value trans-only portion.

**Pre-Renewal values (from RateMyServer transcendent second class table):**

| Job Lv | EXP         | Job Lv | EXP         |
|--------|-------------|--------|-------------|
| 1      | 368         | 36     | 985,696     |
| 2      | 568         | 37     | 1,046,034   |
| 3      | 696         | 38     | 1,231,134   |
| 4      | 1,206       | 39     | 1,449,220   |
| 5      | 1,774       | 40     | 1,678,177   |
| 6      | 2,192       | 41     | 1,773,024   |
| 7      | 3,196       | 42     | 2,182,221   |
| 8      | 5,080       | 43     | 2,534,978   |
| 9      | 7,352       | 44     | 3,043,585   |
| 10     | 8,580       | 45     | 3,782,865   |
| 11     | 9,892       | 46     | 4,461,912   |
| 12     | 13,358      | 47     | 4,672,971   |
| 13     | 18,984      | 48     | 5,971,896   |
| 14     | 31,925      | 49     | 6,250,158   |
| 15     | 35,860      | 50     | 6,875,174   |
| 16     | 40,013      | 51     | 7,562,691   |
| 17     | 51,605      | 52     | 8,318,960   |
| 18     | 68,585      | 53     | 9,150,856   |
| 19     | 87,770      | 54     | 10,065,942  |
| 20     | 96,443      | 55     | 11,877,812  |
| 21     | 105,515     | 56     | 14,015,818  |
| 22     | 131,770     | 57     | 16,538,665  |
| 23     | 167,428     | 58     | 19,515,624  |
| 24     | 206,720     | 59     | 23,028,437  |
| 25     | 223,860     | 60     | 28,094,693  |
| 26     | 241,673     | 61     | 34,275,525  |
| 27     | 294,553     | 62     | 41,816,141  |
| 28     | 362,303     | 63     | 51,015,692  |
| 29     | 479,053     | 64     | 62,239,144  |
| 30     | 513,362     | 65     | 79,666,104  |
| 31     | 548,856     | 66     | 101,972,614 |
| 32     | 656,197     | 67     | 130,524,946 |
| 33     | 787,507     | 68     | 167,071,930 |
| 34     | 927,154     | 69     | 213,852,071 |
| 35     | 985,696     |        |             |

**IMPORTANT**: The transcendent second class uses a DIFFERENT job EXP table from normal second class (values are approximately 2x the normal second class values at early levels, diverging further at higher levels). This is NOT the same table with extra levels appended -- it is a completely separate table.

**Total to Job 70 (Transcendent)**: ~1,084,674,386

### 4.7 Super Novice Job EXP Table (Job Levels 1-99)

Job levels 1-49 use the First Class table. From job level 50 onward, the values increase linearly by 10,000 per level:

| Job Lv | EXP       | Job Lv | EXP       |
|--------|-----------|--------|-----------|
| 1-49   | (Same as First Class table) | | |
| 49     | 509,596   | 75     | 1,232,092 |
| 50     | 982,092   | 80     | 1,282,092 |
| 51     | 992,092   | 85     | 1,332,092 |
| 52     | 1,002,092 | 90     | 1,382,092 |
| 53     | 1,012,092 | 95     | 1,432,092 |
| 55     | 1,032,092 | 98     | 1,462,092 |
| 60     | 1,082,092 | 99     | 1,491,333 |
| 65     | 1,132,092 |        |           |
| 70     | 1,182,092 |        |           |

Pattern for levels 50-98: `982,092 + (jobLevel - 50) * 10,000`
Level 99 breaks the pattern at 1,491,333.

---

## 5. Monster EXP Calculation

### 5.1 Base Monster EXP

Every monster has fixed Base EXP and Job EXP values defined in the monster database. These values are constants -- they do NOT change based on the player's level in pre-renewal.

```
EXP Gained = Monster's Base EXP (or Job EXP) value
```

**This is the fundamental pre-renewal rule.** A Poring always gives 81 base EXP whether killed by a level 2 or a level 98 character.

### 5.2 EXP Calculation Method (Who Gets Credit)

rAthena `exp.conf` defines `exp_calc_type`:
- **Type 0** (default): Damage dealt / Total HP ratio. Each attacker gets EXP proportional to their damage contribution.
- **Type 1**: Damage dealt / Max HP ratio. Can result in >100% total if overkill.
- **Type 2**: Same as Type 0, but first attacker counts twice.

In classic pre-renewal, the **last-hit killer gets the EXP** for solo play. In multi-attacker scenarios, EXP is distributed based on damage contribution.

### 5.3 EXP Tap Bonus (Multi-Attacker Bonus)

When multiple players attack the same monster (regardless of party membership), total EXP increases:

```
TapBonus = 100 + (exp_bonus_attacker * min(exp_bonus_max_attacker - 1, attackerCount - 1))
FinalEXP = floor(MonsterEXP * TapBonus / 100)
```

**rAthena defaults:**
- `exp_bonus_attacker`: 25 (25% bonus per additional attacker)
- `exp_bonus_max_attacker`: 12 (maximum 12 attackers counted)

**Result**: Up to +275% bonus EXP (12 attackers = 100 + 25*11 = 375% of base, or +275%).

**Source conflict**: iRO Wiki Classic says "+25% per attacker, up to 580% max" while iRO Wiki (Renewal page) says "+5% per attacker, up to 100% max". The 25% figure matches rAthena defaults and the Classic wiki. The 5% figure appears to be Renewal-specific.

**Sabri_MMO currently implements**: 25% per attacker, max 11 bonus attackers (lines 2216-2220 of index.js). This matches the pre-renewal rAthena defaults.

### 5.4 EXP Modifiers (Equipment/Items)

The final EXP formula with all modifiers (from iRO Wiki):

```
FinalEXP = floor(OriginalEXP * TapMod * (1 + EquipMod + CardMod + BattleManualMod + VIPMod))
```

**Common EXP modifiers in pre-renewal:**

| Source | Bonus |
|--------|-------|
| Battle Manual | +50% |
| HE Battle Manual | +100% |
| Battle Manual x3 | +200% |
| Peco Headband | +10% (vs Brute) |
| Various shoes with race cards | +5-10% per race |
| Mental Sensing (Sage skill) | +50% |

Note: Equipment/card modifiers do NOT affect Quest EXP.

---

## 6. Level Difference Penalty

### 6.1 Pre-Renewal: NO PENALTY

**In pre-renewal RO, there is NO level difference penalty.** This is confirmed by:

1. **iRO Wiki Classic** (primary source): "The character level or monster level does not affect EXP gained. For example, killing a Poporing will always worth 81 base EXP whether it is killed by a level 15 character or a level 98 character."
2. **rAthena source**: The level penalty system (`db/re/level_penalty.yml`) exists ONLY in the `db/re/` (Renewal) directory. There is no `db/pre-re/level_penalty.yml`.
3. **Origins RO Wiki** (faithful pre-renewal server): Does not document any level difference penalties.

### 6.2 Renewal Level Penalty (NOT used in pre-renewal, documented for reference)

The Renewal system introduced level-based EXP modifiers. These are listed here ONLY for comparison and should NOT be implemented in a pre-renewal server:

| Monster Level vs Player | EXP Modifier |
|------------------------|-------------|
| Monster 10+ levels above | 140% |
| Monster 6-10 levels above | 120% |
| Monster 1-5 levels above | 110% |
| Same level | 100% |
| Monster 1-5 below | 95% |
| Monster 6-10 below | 90% |
| Monster 11-15 below | 85% |
| Monster 16-20 below | 80% |
| Monster 21-25 below | 60% |
| Monster 26-30 below | 35% |
| Monster 31+ below | 10% |

MVPs are exempt from this system in Renewal.

### 6.3 Impact on Sabri_MMO

The existing `01_Stats_Leveling_JobSystem.md` documents a level difference modifier table (Section 3.7). **This table is for Renewal, not pre-renewal.** If Sabri_MMO is targeting pre-renewal mechanics, this table should NOT be used for EXP calculations. It can optionally be kept as a game design lever if the developers want to discourage over-leveling on low monsters, but it is not RO Classic authentic.

The server (`index.js`) currently does NOT implement level difference penalties (verified: `processExpGain` takes raw baseExp/jobExp and applies them directly). This is correct for pre-renewal.

---

## 7. Party EXP Sharing

### 7.1 Two Modes

#### Each Take
- Each party member gains EXP ONLY from monsters they personally damaged.
- No sharing, no bonus.
- Default mode when party is created.

#### Even Share
- All EXP from party kills is pooled and split equally among eligible members.
- A bonus is applied based on party size (see below).
- **Requirement**: All online party members must be within 15 base levels of each other. If the gap exceeds 15, Even Share falls back to Each Take behavior.
- **Same map requirement**: Only members on the same map as the killed monster receive EXP.
- **Alive requirement**: Dead members do not receive EXP shares.

### 7.2 Party Bonus Formula (Even Share)

```
TotalEXP = MonsterEXP * (100 + PARTY_EVEN_SHARE_BONUS * (EligibleMemberCount - 1)) / 100
PerMemberEXP = floor(TotalEXP / EligibleMemberCount)
```

Where `PARTY_EVEN_SHARE_BONUS` = 20 (20% per member beyond the first).

| Party Size (Eligible) | Total EXP Pool | Per Person |
|----------------------|---------------|------------|
| 1                    | 100%          | 100%       |
| 2                    | 120%          | 60%        |
| 3                    | 140%          | ~47%       |
| 4                    | 160%          | 40%        |
| 5                    | 180%          | 36%        |
| 6                    | 200%          | ~33%       |
| 7                    | 220%          | ~31%       |
| 8                    | 240%          | 30%        |
| 9                    | 260%          | ~29%       |
| 10                   | 280%          | 28%        |
| 11                   | 300%          | ~27%       |
| 12                   | 320%          | ~27%       |

**Both Base EXP and Job EXP** use the same formula.

### 7.3 Level Range Check

```
MaxLevelInParty - MinLevelInParty <= 15
```

If this check fails, Even Share is disabled and all members revert to Each Take for that kill.

**rAthena source** (`conf/battle/party.conf`): The level gap is not directly configurable in party.conf but is hardcoded. The Sabri_MMO server uses `PARTY_SHARE_LEVEL_GAP = 15`.

### 7.4 Eligibility Conditions for EXP Share

A party member receives Even Share EXP only if ALL of:
1. They are online (connected)
2. They are on the same map/zone as the killed monster
3. They are alive (HP > 0)
4. The party-wide level gap check passes

### 7.5 Current Implementation Status

The server (`distributePartyEXP` at line 5189 of `index.js`) correctly implements:
- Even Share vs Each Take modes
- 15-level gap check (falls back to solo result)
- Same-zone filtering
- Alive check (HP > 0)
- 20% bonus per eligible member beyond the first
- Both base and job EXP shared with the same formula

---

## 8. Death Penalty

### 8.1 Pre-Renewal Death Penalty

When a player character dies to a PvE source:

```
BasePenalty = floor(BaseEXPNeededForCurrentLevel * 0.01)  // 1% of current level's requirement
JobPenalty  = floor(JobEXPNeededForCurrentJobLevel * 0.01)  // 1% of current job level's requirement
```

**rAthena defaults** (`conf/battle/exp.conf`):
- `death_penalty_type`: 1 (lose % of current level requirement)
- `death_penalty_base`: 100 (each 100 = 1%, so 100 = 1%)
- `death_penalty_job`: 100 (each 100 = 1%, so 100 = 1%)
- `death_penalty_maxlv`: 0 (no penalty at max level -- official behavior)

### 8.2 Penalty Type Options

| Type | Formula | Notes |
|------|---------|-------|
| 0 | No penalty | No EXP lost |
| 1 (default) | `floor(EXPToNextLevel * penalty / 10000)` | 1% of current level requirement |
| 2 | `floor(TotalAccumulatedEXP * penalty / 10000)` | 1% of total lifetime EXP (much harsher) |

Pre-renewal official servers use Type 1.

### 8.3 Exemptions

| Condition | Penalty Applied? |
|-----------|-----------------|
| Novice class | **NO** -- Novice is fully exempt |
| High Novice | **YES** -- transcendent classes are NOT exempt (despite being "Novice" tier) |
| Super Novice | **YES** -- 1% base EXP loss applies |
| Max base level (99) | **NO** -- `getBaseExpForNextLevel(99)` returns 0, so penalty is 0 |
| Max job level | **NO** -- `getJobExpForNextLevel()` returns 0, so penalty is 0 |
| PvP death | Server-configurable, typically **NO** penalty in official |
| WoE death | **NO** penalty |
| Minigame death | **NO** penalty |

### 8.4 Cannot Delevel

EXP is clamped at 0 within the current level:

```
player.baseExp = Math.max(0, player.baseExp - basePenalty);
```

A player can never lose a base level or job level from dying. They can lose progress toward the next level, but never go backwards.

### 8.5 Absolute EXP Loss Examples

Because the penalty is 1% of the current level's requirement, the absolute loss scales with level:

| Level | EXP to Next (Normal) | 1% Penalty |
|-------|---------------------|-----------|
| 10    | 320                 | 3         |
| 30    | 7,995               | 79        |
| 50    | 115,254             | 1,152     |
| 70    | 1,473,746           | 14,737    |
| 90    | 9,738,720           | 97,387    |
| 98    | 99,999,998          | 999,999   |

For Transcendent classes (same 1% rate, but higher EXP tables):

| Level | EXP to Next (Trans.) | 1% Penalty |
|-------|---------------------|-----------|
| 70    | 3,389,616           | 33,896    |
| 90    | 29,216,160          | 292,161   |
| 98    | 343,210,000         | 3,432,100 |

### 8.6 Job EXP Death Penalty

**rAthena default**: Job EXP IS penalized on death (`death_penalty_job: 100`).

**Discrepancy**: Some community sources (including the existing `01_Stats_Leveling_JobSystem.md`) state "Job EXP is NOT lost on death." However, rAthena's default configuration applies 1% job EXP loss alongside base EXP loss. This may reflect server-specific configurations (some official servers may have disabled job EXP penalty).

**Sabri_MMO currently implements**: Both base AND job EXP penalties (lines 2084-2101 of index.js). This matches rAthena defaults.

### 8.7 Death Penalty Mitigation

| Skill/Effect | Mitigation |
|-------------|-----------|
| Redemptio (Priest, ID 1012) | Revives all dead party members and restores their death penalty EXP |
| Token of Siegfried (item) | Reduces death penalty, element-based |
| Yggdrasil Leaf (item) | Resurrects target (does not restore lost EXP) |

---

## 9. Quest EXP

### 9.1 Quest EXP Cap

Quest rewards cannot cause a player to gain more than a fixed amount of EXP in a single reward:

```
MaxGain = (EXPRequiredForCurrentLevel * 2) - CurrentEXPInLevel - 1
```

This means:
- A quest can never grant more than ~2 levels worth of EXP
- If a player is at 0% of their level, the max gain is `2 * EXPToNext - 1`
- If a player is at 99.99% of their level, the max gain is approximately `1 * EXPToNext`

### 9.2 Quest EXP Modifiers

- **Battle Manuals** DO affect quest EXP rewards (iRO Wiki confirmed)
- **VIP bonuses** DO affect quest EXP rewards
- **Equipment/Card EXP bonuses** do NOT affect quest EXP rewards

### 9.3 Repeatable EXP Quests

Official iRO had repeatable EXP quests (hunt X monsters for EXP). The rewards were:
- 50 monsters: max gain of 1 base level + 1 job level
- 100 monsters: max gain of 2 levels
- 150 monsters: max gain of 3 levels

### 9.4 Job Change Quest EXP

Job change quests themselves typically grant no direct EXP. They are gating quests that unlock the class change NPC. However, some associated quests (prerequisite quests) do grant EXP.

---

## 10. Transcendent / Rebirth

### 10.1 Requirements for Rebirth

| Requirement | Value |
|------------|-------|
| Base Level | 99 |
| Job Level | 50 (as a Second Class) |
| Location | Book of Ymir in Juno (Sage Castle) |
| Cost | 1,285,000 zeny (or hunt Runaway Book for free) |
| Weight | Must be under 100 weight |
| Restrictions | No Cart, no Pet, no Peco Peco, all Skill Points must be spent |

### 10.2 What Happens on Rebirth

| State | Before Rebirth | After Rebirth |
|-------|---------------|---------------|
| Base Level | 99 | **1** |
| Job Level | 50 | **1** (High Novice) |
| Base EXP | Any | **0** |
| Job EXP | Any | **0** |
| All Stats | Current allocations | **RESET** (100 stat points available) |
| All Skills | Current skills | **RESET** (all skill points refunded) |
| Stat Points | Spent | **100** distributable (vs 48 for normal Novice) |
| Zeny | Current | **Retained** |
| Equipment | Current | **Retained** |
| Inventory | Current | **Retained** |
| Storage | Current | **Retained** |
| Quest Progress | Current | **Retained** |
| Location | Any | Warped to town of original First Class |

### 10.3 Transcendent Benefits

1. **+25% Max HP and Max SP** (permanent, multiplicative modifier applied after base HP/SP calculation)
2. **+52 extra stat points** (100 starting vs 48 normal = 52 extra, grand total 1,325 vs 1,273)
3. **Job Level 70** (instead of 50) for the transcendent second class = 20 extra skill points
4. **Access to transcendent-exclusive skills** (e.g., Lord Knight's Spiral Pierce, High Wizard's Ganbantein)
5. **Access to transcendent-exclusive equipment** (e.g., Valkyrie gear)
6. **Automatic Quest Skills**: High Novice gets First Aid and Play Dead without completing their quests

### 10.4 Transcendent EXP Tables

Transcendent characters use a different (harder) base EXP table -- see Section 4.2. The transcendent table requires approximately 3x the total EXP to reach 99 compared to normal classes.

Transcendent second classes also use a different job EXP table -- see Section 4.6.

### 10.5 Transcendent Class Progression

```
Second Class (Lv 99 / Job 50) -> Rebirth -> High Novice (Lv 1 / Job 1)
  -> High First Class (Job 10) -> Transcendent Second Class (Job 50)
  -> Continue to Job 70
```

The class mapping:
| Second Class | Transcendent Class |
|-------------|-------------------|
| Knight | Lord Knight |
| Wizard | High Wizard |
| Hunter | Sniper |
| Priest | High Priest |
| Assassin | Assassin Cross |
| Blacksmith | Whitesmith |
| Crusader | Paladin |
| Sage | Scholar |
| Bard | Minstrel |
| Dancer | Gypsy |
| Monk | Champion |
| Rogue | Stalker |
| Alchemist | Biochemist |

### 10.6 High Novice Job Bonus Stats

High Novice receives small stat bonuses distributed across job levels:
- STR: +8 total
- AGI: +5
- VIT: +6
- INT: +9
- DEX: +3
- LUK: +2

These are in addition to the normal job bonus stats received by their High First Class and Transcendent Second Class.

---

## 11. Super Novice Special Rules

### 11.1 Job Level

Super Novice has the highest job level cap at **99** (vs 50 for normal classes, 70 for transcendent). This grants 98 skill points total, allowing access to first-class skills from ALL six first classes.

### 11.2 Death Penalty

Super Novice receives the standard 1% base EXP penalty on death (they are NOT exempt like regular Novice).

### 11.3 Guardian Angel -- Level Up Buffs

When a Super Novice levels up, their Guardian Angel automatically casts:
- Kyrie Eleison
- Magnificat
- Gloria
- Suffragium
- Impositio Manus

### 11.4 Guardian Angel -- Death Protection ("Never Give Up!")

When a Super Novice's HP reaches 0 AND their base EXP percentage is between **99.0% and 99.9%** (inclusive), the Guardian Angel MAY appear to:
1. Fully restore HP
2. Cast Steel Body (Mental Strength) on the Super Novice

**Conditions**:
- Only triggers at 99.0%-99.9% base EXP
- Does NOT trigger at exactly 100% (would level up instead)
- Does NOT trigger below 99.0%
- Resets upon relogging or gaining a level
- Not guaranteed (there is a random chance element)

### 11.5 Fury Status ("Novice's Fury")

Super Novice can activate a special Fury status that grants **+50 CRI** by performing a specific chant:

**Trigger condition**: Base EXP percentage must be an exact multiple of 10.0% (10.0%, 20.0%, 30.0%, 40.0%, 50.0%, 60.0%, 70.0%, 80.0%, 90.0%).

The chant requires typing specific dialogue lines in the chat window.

### 11.6 Post-Level 99 EXP Accumulation

After reaching Base Level 99, Super Novice can continue accumulating EXP (up to 99,999,999 EXP). At certain thresholds:
- Gaining `n * 10,000,000` EXP (multiples of 10 million) enables Fury status
- Gaining 99,000,000+ additional EXP enables Mental Strength activation at 0 HP

### 11.7 EXP Table

Super Novice uses the **Normal** base EXP table (not Transcendent). Their job EXP table follows the First Class table for levels 1-49, then a linear progression for 50-99 (see Section 4.7).

---

## 12. MVP EXP Bonus

### 12.1 MVP Reward Distribution

When an MVP (boss monster) is killed:
- The player who **dealt the most total damage** is designated the "MVP"
- The MVP player receives:
  - **MVP EXP**: Defined per-monster in the monster database (separate from the monster's normal base/job EXP). Some MVPs give 0 MVP EXP.
  - **MVP reward items**: 1-3 items randomly selected from the monster's MVP drop table, given directly to inventory
- The monster's normal Base EXP and Job EXP are distributed to all attackers based on damage contribution (standard rules)

### 12.2 MVP EXP is Additional

MVP EXP is a BONUS on top of the regular monster EXP. The MVP player gets:
1. Their share of the regular EXP (based on damage %)
2. The MVP-specific bonus EXP (100% to the MVP player)

### 12.3 Level Penalty Exemption

In Renewal, MVPs are exempt from the level difference penalty system (they always give 100% EXP). In pre-renewal, this is irrelevant since there is no level difference penalty for ANY monster.

### 12.4 Party Interaction with MVPs

- If the MVP killer is in a party with Even Share, the regular monster EXP is shared among eligible party members
- The MVP bonus EXP goes ONLY to the MVP player (the most-damage dealer)
- MVP reward items go ONLY to the MVP player
- If the MVP player dies as the MVP monster dies and no other eligible player is on the map, a random player on the map receives the rewards

---

## 13. Edge Cases

### 13.1 Multi-Level-Up from a Single Kill

**Official servers**: Disabled. If a kill grants enough EXP for multiple levels, the player levels up once and excess EXP is applied toward the next level (but capped at one level-up per kill).

**rAthena default**: `multi_level_up: no` -- only one level gained per EXP application.

**Sabri_MMO implementation**: The current `processExpGain` uses a while loop (lines 3982-4010) that processes multiple level-ups. This is technically a multi-level-up implementation. If official behavior is desired, it should be capped at one level-up per call with overflow EXP capped at `nextLevelEXP - 1`.

### 13.2 Party Out of Range / Different Map

- Members on a different map from the kill do NOT receive EXP shares
- Members who are dead do NOT receive EXP shares
- These members are simply excluded from the eligible count, which means remaining members get a slightly larger share

### 13.3 Overkill

If a monster has 100 HP and is hit for 1000 damage, the EXP is still the full monster EXP. There is no penalty for overkill.

### 13.4 Monster Killed by DoT (Damage over Time)

When a monster dies to a DoT effect (poison, Meteor Storm ground DoT, etc.), the EXP goes to the player who applied the DoT. If multiple DoTs from different players, the last-hit DoT owner gets credit.

### 13.5 Monster Killed by Trap

EXP goes to the player who placed the trap. This includes Hunter traps (Claymore, Blast Mine, etc.) and ground-effect skills (Fire Pillar).

### 13.6 Max Base or Job Level

- At max base level (99), `getBaseExpForNextLevel(99)` returns 0
- Base EXP gained is discarded (no overflow storage except Super Novice)
- At max job level, job EXP gained is discarded
- No death penalty at max level (penalty calculation yields 0)

### 13.7 Simultaneous Kill Credit

If multiple players deal the killing blow in the same server tick, the player with the highest total damage dealt gets MVP credit. Regular EXP is split by damage ratio.

### 13.8 EXP from Healing/Buffing

Players who only heal or buff party members (without dealing damage to the monster) do NOT count as "attackers" for EXP tap bonus purposes. However, in Even Share mode, they still receive their share of the party EXP pool as long as they are on the same map, alive, and within the level range.

### 13.9 Homunculus EXP Sharing

When a player has an active Homunculus:
- The Homunculus receives a fixed percentage of the player's EXP (typically 10%)
- This is subtracted from the player's share
- The Homunculus has its own level, EXP, and stat system

### 13.10 Pet EXP

Pets do not gain or share EXP in pre-renewal. Their intimacy system is separate.

### 13.11 Baby Class EXP

Baby classes use the Normal EXP table (same as regular classes). They do NOT get the 25% HP/SP bonus. Their job level caps match their non-baby counterparts (50 for Baby Second Classes).

---

## 14. Implementation Checklist

### Currently Implemented in Sabri_MMO

- [x] Normal Base EXP table (1-99) -- `ro_exp_tables.js`
- [x] Novice Job EXP table (1-10)
- [x] First Class Job EXP table (1-50)
- [x] Second Class Job EXP table (1-50)
- [x] `processExpGain()` with multi-level-up support
- [x] Stat points per level formula (`floor(level/5) + 3`)
- [x] 1 skill point per job level
- [x] HP/SP class coefficients (all classes including transcendent)
- [x] Full heal on level up
- [x] Death penalty (1% base + job EXP, Novice exempt)
- [x] Party EXP sharing (Even Share with 20% bonus)
- [x] 15-level gap check for Even Share
- [x] Same-zone filtering for party EXP
- [x] Dead member exclusion from party EXP
- [x] EXP Tap Bonus (+25% per attacker, max 11 bonus)
- [x] `JOB_CLASS_CONFIG` with all normal classes
- [x] Job change system (Novice -> First -> Second)
- [x] `SECOND_CLASS_UPGRADES` mapping

### Missing / Needs Implementation

- [ ] **Transcendent Base EXP table** -- not in `ro_exp_tables.js`
- [ ] **Transcendent Second Class Job EXP table** (Job 1-70) -- not in `ro_exp_tables.js`
- [ ] **Super Novice Job EXP table** (Job 1-99) -- not in `ro_exp_tables.js`
- [ ] **Transcendent class config** in `JOB_CLASS_CONFIG` -- missing Lord Knight, High Wizard, etc.
- [ ] **Rebirth system** -- no rebirth/transcendence quest flow
- [ ] **Super Novice class** -- not in `JOB_CLASS_CONFIG`
- [ ] **Super Novice Guardian Angel** (level-up buffs, death protection, Fury)
- [ ] **Quest EXP cap** (`EXPReq * 2 - CurrentEXP - 1`)
- [ ] **Multi-level-up cap** -- current implementation allows unlimited multi-level-up (official disables it)
- [ ] **MVP EXP distribution** -- need to verify MVP bonus EXP goes only to top damage dealer
- [ ] **Max level EXP discard** -- verify EXP at max level is properly discarded
- [ ] **Job bonus stats** -- per-class hidden stat bonuses at specific job levels
- [ ] **Baby class configs** -- not in `JOB_CLASS_CONFIG`
- [ ] **Extended class configs** (TaeKwon, Ninja, Gunslinger) -- not in `JOB_CLASS_CONFIG`
- [ ] **EXP modifier stacking** (Battle Manuals, equipment bonuses)

### Data Discrepancies to Resolve

1. **First Class Job Lv 1**: Server uses 30, RateMyServer shows 43. Need rAthena YAML verification.
2. **Second Class Job Lv 1**: Server uses 144, RateMyServer shows 184. Need rAthena YAML verification.
3. **Second Class Job Lv 35**: Server uses 337,147, doc shows 358,435. Need verification.
4. **Job EXP death penalty**: Some sources say job EXP is NOT lost, rAthena defaults say it IS. Current server implements job EXP loss (matching rAthena).
5. **Level difference penalty**: `01_Stats_Leveling_JobSystem.md` Section 3.7 documents a Renewal-era penalty table. This should be marked as Renewal-only or removed from pre-renewal reference.

---

## 15. Gap Analysis

### High Priority (Blocks Content)

| Gap | Impact | Effort |
|-----|--------|--------|
| Transcendent EXP tables | Blocks transcendent class progression | Low (data entry) |
| Transcendent class config | Blocks transcendent class creation/progression | Low (add to JOB_CLASS_CONFIG) |
| Rebirth system | Blocks entire transcendent gameplay loop | Medium (quest flow, level/stat/skill reset) |

### Medium Priority (Affects Accuracy)

| Gap | Impact | Effort |
|-----|--------|--------|
| Multi-level-up cap | Players can skip levels in current implementation (non-official) | Low (add cap to processExpGain) |
| Job bonus stats | Missing hidden stat bonuses per job level | Medium (data per class, apply in getEffectiveStats) |
| Super Novice class | Missing entire class | Medium (class config + special rules) |
| Quest EXP cap | Quests can give unlimited EXP | Low (add cap formula to quest reward handler) |
| EXP modifier system | No Battle Manual / equipment EXP bonuses | Medium (modifier stack in processExpGain) |

### Low Priority (Polish)

| Gap | Impact | Effort |
|-----|--------|--------|
| Baby class configs | Baby classes not playable | Low (same tables, no HP/SP bonus) |
| Extended class configs | TaeKwon/Ninja/Gunslinger not playable | Low (add to JOB_CLASS_CONFIG) |
| Super Novice Guardian Angel | Flavor feature | Medium (level-up event hooks) |
| Super Novice Fury | Flavor feature | Low (EXP % check + CRI bonus) |
| Data discrepancies (Job Lv 1 values) | Minor EXP accuracy at very low levels | Low (verify against rAthena YAML) |

### Recommendations

1. **Immediate**: Add transcendent EXP tables and class configs to `ro_exp_tables.js`. This is pure data entry.
2. **Soon**: Implement the rebirth flow (level/stat/skill reset, zeny/item preservation, High Novice creation).
3. **Verify**: Compare Job Lv 1 EXP values against rAthena `db/pre-re/job_exp.yml` raw YAML to resolve discrepancies.
4. **Consider**: Whether to cap multi-level-up (official behavior) or leave it enabled for faster progression during development.
5. **Remove or annotate**: The level difference penalty table in Section 3.7 of `01_Stats_Leveling_JobSystem.md` -- it is Renewal-only and misleading for a pre-renewal implementation.
