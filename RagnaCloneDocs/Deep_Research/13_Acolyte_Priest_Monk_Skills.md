# Acolyte / Priest / Monk Skills -- Deep Research (Pre-Renewal)

> **Sources**: iRO Wiki Classic, RateMyServer Pre-Renewal DB, rAthena pre-re skill_db.yml, TalonRO Wiki, divine-pride.net, Ragnarok Fandom Wiki, GameFAQs Monk/Priest guides
> **Scope**: All skills for Acolyte (IDs 400-414), Priest (IDs 1000-1018), Monk (IDs 1600-1615)
> **Date**: 2026-03-22

---

## Table of Contents

1. [Acolyte Skills (IDs 400-414)](#acolyte-skills-ids-400-414)
2. [Priest Skills (IDs 1000-1018)](#priest-skills-ids-1000-1018)
3. [Monk Skills (IDs 1600-1615)](#monk-skills-ids-1600-1615)
4. [Healing System](#healing-system)
5. [Buff System](#buff-system)
6. [Exorcism Skills](#exorcism-skills)
7. [Resurrection Mechanics](#resurrection-mechanics)
8. [Safety Wall Mechanics](#safety-wall-mechanics)
9. [Spirit Sphere System](#spirit-sphere-system)
10. [Combo System](#combo-system)
11. [Steel Body Mechanics](#steel-body-mechanics)
12. [Sitting / Meditation System](#sitting--meditation-system)
13. [Skill Trees and Prerequisites](#skill-trees-and-prerequisites)
14. [Implementation Checklist](#implementation-checklist)
15. [Gap Analysis](#gap-analysis)

---

## Acolyte Skills (IDs 400-414)

### Heal (ID 400)

| Property | Value |
|----------|-------|
| rAthena ID | 28 (AL_HEAL) |
| Type | Supportive, single target |
| Max Level | 10 |
| Target | Self / Ally / Undead enemy |
| Range | 9-10 cells |
| Cast Time | 0 (instant) |
| After-Cast Delay | 1000ms |
| Element | Holy (vs Undead) |

**Heal Formula (Pre-Renewal):**
```
HealAmount = floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
```

The factor `(4 + 8 * SkillLv)` yields base factors of 12, 20, 28, 36, 44, 52, 60, 68, 76, 84 for levels 1-10.

**SP Cost:** `10 + 3 * SkillLv` = 13, 16, 19, 22, 25, 28, 31, 34, 37, 40

**Undead Damage:** When targeting an Undead element enemy, deals Holy damage equal to `floor(HealAmount / 2)`, then modified by Holy vs Undead element table. Ignores MDEF.

**Modifiers:**
- Equipment Heal Power bonus (`bHealPower`) applies as percentage increase
- Card Heal Power bonus applies additively
- Heal received modifiers on target also apply

**Example:** BaseLv 99, INT 99, Lv10: `floor(198/8) * 84 = 24 * 84 = 2016 HP`

---

### Divine Protection (ID 401)

| Property | Value |
|----------|-------|
| rAthena ID | 22 (AL_DPPRO) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Effect:** Reduces physical damage from Undead property AND Demon race monsters.

**Formula:** `(3 * SkillLv) + floor(0.04 * (BaseLv + 1))` soft DEF reduction

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Base DEF | +3 | +6 | +9 | +12 | +15 | +18 | +21 | +24 | +27 | +30 |

The base level component adds an extra ~4 DEF at level 99. Applied as soft DEF (VIT-like, subtracted after hard DEF reductions). Does NOT work against players.

---

### Demon Bane (ID 413)

| Property | Value |
|----------|-------|
| rAthena ID | 23 (AL_DEMONBANE) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Divine Protection Lv3 |

**Effect:** Increases ATK against Undead property AND Demon race monsters.

**Formula:** `(3 * SkillLv) + floor(0.05 * (BaseLv + 1))` mastery ATK bonus

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Base ATK | +3 | +6 | +9 | +12 | +15 | +18 | +21 | +24 | +27 | +30 |

Mastery ATK (ignores armor DEF, like Sword Mastery). Base level component adds ~5 ATK at level 99.

---

### Blessing (ID 402)

| Property | Value |
|----------|-------|
| rAthena ID | 34 (AL_BLESSING) |
| Type | Supportive, single target |
| Max Level | 10 |
| Target | Self / Ally / Enemy (Undead/Demon) |
| Range | 9 cells |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Prerequisites | Divine Protection Lv5 |

**Friendly Target:** +STR/DEX/INT per level.
**Undead/Demon Enemy:** Halves target's STR, DEX, INT (regardless of skill level). Boss immune.
**Curse/Stone Cure:** Removes Curse and Stone (stage 2) from target. When curing, target does NOT receive stat bonuses.

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 | 64 |
| STR/DEX/INT+ | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Duration(s) | 60 | 80 | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |

**SP Formula:** `24 + 4 * SkillLv`
**Duration Formula:** `40 + 20 * SkillLv` seconds

---

### Increase AGI (ID 403)

| Property | Value |
|----------|-------|
| rAthena ID | 29 (AL_INCAGI) |
| Type | Supportive, single target |
| Max Level | 10 |
| Target | Self / Ally |
| Range | 9 cells |
| Cast Time | 1000ms (variable, reduced by DEX) |
| After-Cast Delay | 1000ms |
| HP Cost | 15 (flat, all levels) |
| Min HP Check | Fails if caster HP < 16 |
| Prerequisites | Heal Lv3 |

**Effect:** Increases AGI and movement speed. Cancels Decrease AGI on target.

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 18 | 21 | 24 | 27 | 30 | 33 | 36 | 39 | 42 | 45 |
| AGI+ | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Duration(s) | 60 | 80 | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |

**SP Formula:** `15 + 3 * SkillLv`
**Move Speed:** +25% movement speed while active.
**Buff Dispel:** Removes Decrease AGI on application.

---

### Decrease AGI (ID 404)

| Property | Value |
|----------|-------|
| rAthena ID | 30 (AL_DECAGI) |
| Type | Debuff, single target |
| Max Level | 10 |
| Target | Enemy only |
| Range | 9 cells |
| Cast Time | 1000ms |
| After-Cast Delay | 1000ms |
| Prerequisites | Increase AGI Lv1 |

**Success Rate Formula:** `40 + 2 * SkillLv + floor((BaseLv + INT) / 5) - TargetMDEF` %

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31 | 33 |
| AGI- | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Base Success% | 42 | 44 | 46 | 48 | 50 | 52 | 54 | 56 | 58 | 60 |
| Duration Mon(s) | 30 | 40 | 50 | 60 | 70 | 80 | 90 | 100 | 110 | 120 |
| Duration PvP(s) | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 65 |

**Boss Immunity:** Does NOT work on Boss monsters.
**Move Speed:** -25% movement speed.
**Cancels Buffs:** Increase AGI, Adrenaline Rush, Two-Hand Quicken, Spear Quicken, Cart Boost.
**SP still consumed on failure.**

---

### Cure (ID 405)

| Property | Value |
|----------|-------|
| rAthena ID | 35 (AL_CURE) |
| Type | Supportive, single target |
| Max Level | 1 |
| Target | Self / Ally / Enemy |
| SP Cost | 15 |
| Cast Time | 0 |
| Range | 9 cells |
| Prerequisites | Heal Lv2 |

**Effect:** Removes Silence, Blind, and Confusion (Chaos) status effects.

---

### Angelus (ID 406)

| Property | Value |
|----------|-------|
| rAthena ID | 33 (AL_ANGELUS) |
| Type | Supportive, party-wide |
| Max Level | 10 |
| Target | Self + All party members on screen |
| Cast Time | 500ms |
| After-Cast Delay | 3500ms |
| Prerequisites | Divine Protection Lv3 |

**Effect:** Increases VIT-based soft DEF by a percentage.

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 23 | 26 | 29 | 32 | 35 | 38 | 41 | 44 | 47 | 50 |
| VIT DEF+% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| Duration(s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

**Note:** Increases soft DEF only, does NOT increase VIT stat directly or VIT-related bonuses.

---

### Signum Crucis (ID 407)

| Property | Value |
|----------|-------|
| rAthena ID | 32 (AL_CRUCIS) |
| Type | AoE Debuff |
| Max Level | 10 |
| Target | All Undead element AND Demon race monsters on screen |
| SP Cost | 35 (all levels) |
| Cast Time | 500ms |
| After-Cast Delay | 2000ms |
| Duration | Permanent (until monster dies) |
| AoE | Screen-wide (~15 cells radius) |
| Prerequisites | Demon Bane Lv3 |

**Success Rate Formula:** `23 + 4 * SkillLv + CasterBaseLv - TargetLv` %

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| DEF-% | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| Base Success% | 27 | 31 | 35 | 39 | 43 | 47 | 51 | 55 | 59 | 63 |

**Works on bosses.** Affected monsters perform `/swt` emote on success.

---

### Ruwach (ID 408)

| Property | Value |
|----------|-------|
| rAthena ID | 24 (AL_RUWACH) |
| Type | Self buff with AoE detection |
| Max Level | 1 |
| AoE | 5x5 cells |
| SP Cost | 10 |
| Duration | 10 seconds |
| Damage | 145% MATK Holy to revealed hidden enemies |

---

### Teleport (ID 409)

| Property | Value |
|----------|-------|
| rAthena ID | 26 (AL_TELEPORT) |
| Type | Self |
| Max Level | 2 |
| SP Cost | Lv1: 10, Lv2: 9 |
| Prerequisites | Ruwach Lv1 |

**Lv1:** Random location on current map.
**Lv2:** Teleport to save point.
Cannot be used in PvP/WoE maps.

---

### Warp Portal (ID 410)

| Property | Value |
|----------|-------|
| rAthena ID | 27 (AL_WARP) |
| Type | Ground |
| Max Level | 4 |
| Range | 9 cells |
| Cast Time | 1000ms |
| After-Cast Delay | 1000ms |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Teleport Lv2 |

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| SP | 35 | 32 | 29 | 26 |
| Duration(s) | 10 | 15 | 20 | 25 |
| Memo Slots | 0 | 1 | 2 | 3 |

Lv1 = save point only. Lv2+ = memorized locations via `/memo` command. Max 3 active portals.

---

### Pneuma (ID 411)

| Property | Value |
|----------|-------|
| rAthena ID | 25 (AL_PNEUMA) |
| Type | Ground (3x3) |
| Max Level | 1 |
| SP Cost | 10 |
| Duration | 10 seconds |
| Range | 9 cells |
| Prerequisites | Warp Portal Lv4 |

**Effect:** Blocks ALL ranged physical attacks (range 4+ cells). Does NOT block magic or melee.
**Overlap Rule:** Cannot overlap Pneuma, Safety Wall, or Magnetic Earth.

---

### Aqua Benedicta (ID 412)

| Property | Value |
|----------|-------|
| rAthena ID | 31 (AL_HOLYWATER) |
| Type | Active, Self |
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 1000ms |
| After-Cast Delay | 500ms |
| Catalyst | 1 Empty Bottle (consumed) |

**Effect:** Creates 1 Holy Water. Must stand on water cell or Deluge ground effect.

---

### Holy Light (ID 414) -- Quest Skill

| Property | Value |
|----------|-------|
| rAthena ID | 156 (AL_HOLYLIGHT) |
| Type | Offensive Magic |
| Max Level | 1 |
| Target | Single enemy |
| Range | 9 cells |
| Element | Holy |
| SP Cost | 15 |
| Cast Time | 2000ms |
| Damage | 125% MATK |

**Special:** Cancels Kyrie Eleison on target even if Holy Light misses. Quest skill, not learnable via skill points.

---

## Priest Skills (IDs 1000-1018)

### Sanctuary (ID 1000)

| Property | Value |
|----------|-------|
| rAthena ID | 70 (PR_SANCTUARY) |
| Type | Ground AoE healing zone |
| Max Level | 10 |
| Target | Ground 5x5 |
| Range | 9 cells |
| Cast Time | 5000ms (variable, reduced by DEX). Renewal: 1s fixed + 4s variable. |
| After-Cast Delay | 2000ms |
| Catalyst | 1 Blue Gemstone |
| Heal Interval | 1 tick per second |
| Prerequisites | Heal Lv1 (Acolyte) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 15 | 18 | 21 | 24 | 27 | 30 | 33 | 36 | 39 | 42 |
| Heal/Tick | 100 | 200 | 300 | 400 | 500 | 600 | 777 | 777 | 777 | 777 |
| Duration(s) | 4 | 7 | 10 | 13 | 16 | 19 | 22 | 25 | 28 | 31 |
| Target Limit | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |

**SP Formula:** `12 + 3 * SkillLv`
**Duration Formula:** `1 + 3 * SkillLv` seconds
**Heal Cap:** 777 at Lv7+, does not increase further.
**Target Limit Formula:** `3 + SkillLv`
**Undead/Demon Damage:** Half of heal value as Holy damage, 2-cell knockback away from center.
**Heal is flat** -- unaffected by MATK, but equipment heal power bonuses apply.
**Overlap:** New Sanctuary replaces caster's previous one (max 1 per caster). Cannot overlap Safety Wall.

---

### Kyrie Eleison (ID 1001)

| Property | Value |
|----------|-------|
| rAthena ID | 73 (PR_KYRIE) |
| Type | Supportive, damage barrier |
| Max Level | 10 |
| Target | Self / Ally |
| Range | 9 cells |
| Cast Time | 2000ms (variable). Renewal: 0.4s fixed + 1.6s variable. |
| After-Cast Delay | 2000ms |
| Duration | 120 seconds (all levels) |
| Prerequisites | Angelus Lv2 (Acolyte) |

**Barrier Formula:** `MaxHP * (10 + 2 * SkillLv) / 100`

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 20 | 20 | 25 | 25 | 25 | 30 | 30 | 30 | 35 |
| MaxHP% Shield | 12% | 14% | 16% | 18% | 20% | 22% | 24% | 26% | 28% | 30% |
| Max Hits | 5 | 6 | 6 | 7 | 7 | 8 | 8 | 9 | 9 | 10 |

**SP Formula:** `20 + 5 * floor((SkillLv - 1) / 3)`
**Hits Formula:** `floor(SkillLv / 2) + 5`

**Mechanics:**
- Blocks physical damage only (melee and ranged). Magic passes through.
- Misses and Lucky Dodges do NOT consume hits.
- Barrier breaks when either HP durability is depleted OR hit limit is reached.
- The hit that breaks it is fully absorbed (always blocks at least 1 hit).
- Holy Light cancels Kyrie on the target even if it misses.
- Removed by Lex Divina, Dispel.
- Defense priority: Perfect Dodge > Flee > Safety Wall > Kyrie Eleison > Pneuma.

---

### Magnificat (ID 1002)

| Property | Value |
|----------|-------|
| rAthena ID | 74 (PR_MAGNIFICAT) |
| Type | Supportive, party-wide |
| Max Level | 5 |
| Target | Self + party members on screen |
| SP Cost | 40 (all levels) |
| Cast Time | 4000ms |
| After-Cast Delay | 2000ms |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 30 | 45 | 60 | 75 | 90 |

**Duration Formula:** `15 + 15 * SkillLv` seconds
**Effect:** Doubles natural HP and SP regeneration rate for all party members on screen.

---

### Gloria (ID 1003)

| Property | Value |
|----------|-------|
| rAthena ID | 75 (PR_GLORIA) |
| Type | Supportive, party-wide |
| Max Level | 5 |
| Target | Self + party members on screen |
| SP Cost | 20 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 2000ms |
| Prerequisites | Kyrie Eleison Lv4, Magnificat Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 10 | 15 | 20 | 25 | 30 |

**Effect:** +30 LUK (flat, all levels, does not scale with level).
**Duration Formula:** `5 + 5 * SkillLv` seconds

---

### Resurrection (ID 1004)

| Property | Value |
|----------|-------|
| rAthena ID | 54 (ALL_RESURRECTION) |
| Type | Supportive, single target |
| Max Level | 4 |
| Target | Dead player |
| Range | 9 cells |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Heal Lv4 (Aco), Inc SP Recovery Lv4 |

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| SP | 60 | 60 | 60 | 60 |
| Cast Time(s) | 6 | 4 | 2 | 0 (instant) |
| HP Restored | 10% | 30% | 50% | 80% |

**Mechanics:**
- Target must be a dead player character.
- Consumes 1 Blue Gemstone per cast.
- Revived player appears at current position (not save point).
- Cannot resurrect if target has already self-respawned.

---

### Magnus Exorcismus (ID 1005)

| Property | Value |
|----------|-------|
| rAthena ID | 79 (PR_MAGNUS) |
| Type | Offensive Magic, ground AoE |
| Max Level | 10 |
| Target | Ground 7x7 (cross-shaped pattern) |
| Range | 9 cells |
| Element | Holy |
| Cast Time | 15000ms (variable). Renewal: 1s fixed + 4s variable. |
| After-Cast Delay | 4000ms |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Turn Undead Lv3, Lex Aeterna Lv1, Safety Wall Lv1 |
| Target Restriction | Only damages Undead element AND Demon race enemies |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 40 | 42 | 44 | 46 | 48 | 50 | 52 | 54 | 56 | 58 |
| Waves | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Duration(s) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |

**SP Formula:** `38 + 2 * SkillLv`
**Duration Formula:** `4 + SkillLv` seconds

**Damage Per Wave:**
- Base: 100% MATK Holy damage per hit
- vs Demon race, Undead race, Shadow property, or Undead property: 130% MATK Holy damage per hit

**Immunity Mechanic:** After being hit by a wave, each monster becomes immune for 3 seconds. This means a single target can be hit by a maximum of 5 waves regardless of skill level (since 5 * 3s = 15s > 14s max duration).

**Wave Bundling:** All damage of each wave is connected in a single bundle despite the animation. Lex Aeterna doubles the entire wave bundle.

---

### Turn Undead (ID 1006)

| Property | Value |
|----------|-------|
| rAthena ID | 77 (PR_TURNUNDEAD) |
| Type | Offensive Magic |
| Max Level | 10 |
| Target | Single enemy (Undead element only) |
| Range | 5 cells |
| Element | Holy (piercing damage) |
| SP Cost | 20 (all levels) |
| Cast Time | 1000ms (variable). Renewal: 0.2s fixed + 0.8s variable. |
| After-Cast Delay | 3000ms |
| Prerequisites | Resurrection Lv1, Lex Divina Lv1 |

**Instant Kill Chance Formula (Pre-Renewal):**
```
Chance% = [(20 * SkillLv) + BaseLv + INT + LUK + {1 - (TargetHP / TargetMaxHP)} * 200] / 1000
```

**Cap:** 70% maximum.

**Lower target HP = higher kill chance.** The `{1 - HP/MaxHP} * 200` term adds up to 200 when HP is near 0.

| Level | Base Chance (stats=0, full HP) |
|-------|------|
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

**Example at Lv10, INT 99, LUK 30, BaseLv 99, target at full HP:**
`(200 + 99 + 99 + 30 + 0) / 1000 = 0.428` = 42.8%

**Example at Lv10, target at 10% HP:**
`(200 + 99 + 99 + 30 + 180) / 1000 = 0.608` = 60.8%

**Fail Damage Formula:**
```
FailDamage = (BaseLv + INT + SkillLv * 10) * 2
```

At BaseLv 99, INT 99, Lv10: `(99 + 99 + 100) * 2 = 596` Holy piercing damage (ignores MDEF).

**Boss Monsters:** Can ONLY receive fail-damage. Instant kill never triggers on bosses even if Undead.

---

### Lex Aeterna (ID 1007)

| Property | Value |
|----------|-------|
| rAthena ID | 78 (PR_LEXAETERNA) |
| Type | Debuff, single target |
| Max Level | 1 |
| Target | Single enemy |
| Range | 9 cells |
| SP Cost | 10 |
| Cast Time | 0 |
| After-Cast Delay | 3000ms |
| Prerequisites | Lex Divina Lv5 |
| Duration | Until consumed (infinite) |

**Effect:** The next damage source deals DOUBLE damage, then the debuff is removed.
- Consumed by first real damage (physical, magical, skill, DoT).
- NOT consumed by 0-damage events, misses, or blocked hits.
- Cannot apply to Frozen or Stone Cursed targets (they already have 125% magic vulnerability).

---

### Mace Mastery (ID 1008)

| Property | Value |
|----------|-------|
| rAthena ID | 65 (PR_MACEMASTERY) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Effect:** +3 ATK per level with Mace weapons (+3 to +30).
Mastery ATK (flat, ignores armor DEF). Same pattern as Sword Mastery.

---

### Impositio Manus (ID 1009)

| Property | Value |
|----------|-------|
| rAthena ID | 66 (PR_IMPOSITIO) |
| Type | Supportive, single target buff |
| Max Level | 5 |
| Target | Self / Ally |
| Range | 9 cells |
| Cast Time | 0 |
| Duration | 60 seconds (all levels) |
| Prerequisites | Mace Mastery Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 13 | 16 | 19 | 22 | 25 |
| +ATK | 5 | 10 | 15 | 20 | 25 |

**SP Formula:** `10 + 3 * SkillLv`
**ATK is mastery-type** (flat, bypasses armor DEF).

---

### Suffragium (ID 1010)

| Property | Value |
|----------|-------|
| rAthena ID | 67 (PR_SUFFRAGIUM) |
| Type | Supportive, single target buff |
| Max Level | 3 |
| Target | Ally ONLY (cannot self-cast) |
| Range | 9 cells |
| SP Cost | 8 (all levels) |
| Cast Time | 0 |
| Prerequisites | Impositio Manus Lv2 |

| Level | 1 | 2 | 3 |
|-------|---|---|---|
| Cast Reduction | 15% | 30% | 45% |
| Duration(s) | 30 | 20 | 10 |

**Consumed on next spell cast.** Higher levels = shorter duration but stronger reduction.

---

### Aspersio (ID 1011)

| Property | Value |
|----------|-------|
| rAthena ID | 68 (PR_ASPERSIO) |
| Type | Supportive, weapon endow |
| Max Level | 5 |
| Target | Self / Ally |
| Range | 9 cells |
| Cast Time | 1000-2000ms |
| Catalyst | 1 Holy Water (ID 523) |
| Prerequisites | Aqua Benedicta Lv1 (Aco), Impositio Manus Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 14 | 18 | 22 | 26 | 30 |
| Duration(s) | 60 | 90 | 120 | 150 | 180 |

**Effect:** Endows target's weapon with Holy element. Overrides weapon's natural element.
**Cancels other element endows** (Endow Blaze, Endow Tsunami, etc.).
**Undead armor:** If target has Undead element armor, Aspersio fails and deals Holy damage instead.

---

### B.S. Sacramenti (ID 1012)

| Property | Value |
|----------|-------|
| rAthena ID | 69 (PR_BENEDICTIO) |
| Type | Supportive, ground 3x3 |
| Max Level | 5 |
| SP Cost | 20 (all levels) |
| Cast Time | 3000ms |
| Requirement | 2 Acolyte-class characters standing adjacent to caster |
| Prerequisites | Aspersio Lv3, Gloria Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 40 | 80 | 120 | 160 | 200 |

**Effect:** Endows armor of all players in 3x3 area with Holy element.
Holy armor: strong vs Shadow/Undead attacks, weak vs Shadow element attacks.

---

### Slow Poison (ID 1013)

| Property | Value |
|----------|-------|
| rAthena ID | 71 (PR_SLOWPOISON) |
| Type | Supportive |
| Max Level | 4 |
| Target | Self / Ally |

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| SP | 6 | 8 | 10 | 12 |
| Duration(s) | 10 | 20 | 30 | 40 |

**Effect:** Temporarily halts poison HP drain. Does NOT cure poison -- poison resumes when buff expires.

---

### Status Recovery (ID 1014)

| Property | Value |
|----------|-------|
| rAthena ID | 72 (PR_STRECOVERY) |
| Type | Supportive |
| Max Level | 1 |
| Target | Self / Ally |
| SP Cost | 5 |
| Cast Time | 0 (some sources say 2s) |
| Range | 9 cells |

**Effect:** Cures Frozen, Stone (all stages), and Stun.

---

### Lex Divina (ID 1015)

| Property | Value |
|----------|-------|
| rAthena ID | 76 (PR_LEXDIVINA) |
| Type | Debuff / Cure |
| Max Level | 10 |
| Target | Single enemy or ally |
| Range | 5 cells |
| Cast Time | 0 |
| After-Cast Delay | 3000ms |
| Prerequisites | Ruwach Lv1 (Acolyte) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 20 | 20 | 20 | 20 | 18 | 16 | 14 | 12 | 10 |
| Silence Duration(s) | 30 | 35 | 40 | 45 | 50 | 50 | 50 | 50 | 55 | 60 |

**Dual-purpose:** If target is already silenced, REMOVES silence instead of applying it.
**100% success rate** in pre-renewal (no resistance check).

---

### Increase SP Recovery (ID 1016)

| Property | Value |
|----------|-------|
| rAthena ID | Priest version |
| Type | Passive |
| Max Level | 10 |

**Effect:** +3 SP regen per level (+3 to +30). Same as Mage version (ID 204).

---

### Safety Wall (Priest) (ID 1017)

| Property | Value |
|----------|-------|
| rAthena ID | 12 (MG_SAFETYWALL) -- shared with Mage |
| Type | Ground 1x1 cell |
| Max Level | 10 |
| Range | 9 cells |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Increase SP Recovery (Priest) Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 30 | 30 | 30 | 35 | 35 | 35 | 40 | 40 | 40 | 40 |
| Cast Time(ms) | 4000 | 3600 | 3200 | 2800 | 2400 | 2000 | 1600 | 1200 | 800 | 400 |
| Hits Blocked | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
| Duration(s) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| Base Durability | 300 | 600 | 900 | 1200 | 1500 | 1800 | 2100 | 2400 | 2700 | 3000 |

**Durability Formula (pre-renewal):**
```
Total HP = 300 * SkillLv + 7000 * (1 + 0.1 * JobLv / 50) + 65 * INT + MaxSP
```

**Mechanics:**
- Blocks all melee physical attacks (auto-attacks and melee skills).
- Ranged and magic attacks pass through.
- The hit that breaks it is fully absorbed.
- Breaks when either hits are depleted OR durability HP is depleted.
- Cannot overlap with Pneuma or Magnetic Earth.
- Cannot stack multiple Safety Walls on the same cell.

---

### Redemptio (ID 1018) -- Quest Skill

| Property | Value |
|----------|-------|
| rAthena ID | 1014 (PR_REDEMPTIO) |
| Type | Self-centered AoE (party only) |
| Max Level | 1 |
| SP Cost | 400 |
| Cast Time | 4000ms (uninterruptible) |
| AoE | 15x15 cells |

**Effect:** Resurrects all dead party members in range at 50% MaxHP.
**Caster Penalty:** HP and SP drop to 1.
**EXP Cost:** 1% of required base EXP, reduced by 0.2% per member resurrected.
**Quest skill** -- must be learned via quest, not skill points.

---

## Monk Skills (IDs 1600-1615)

### Iron Fists (ID 1600)

| Property | Value |
|----------|-------|
| rAthena ID | 259 (MO_IRONHAND) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Divine Protection Lv10, Demon Bane Lv10 |

**Effect:** +3 ATK per level with Knuckle weapons or bare hands (+3 to +30).
Mastery ATK (flat, ignores armor DEF).

---

### Summon Spirit Sphere (ID 1601)

| Property | Value |
|----------|-------|
| rAthena ID | 261 (MO_CALLSPIRITS) |
| Type | Active, Self |
| Max Level | 5 |
| SP Cost | 8 (all levels) |
| Cast Time | 1000ms (uninterruptible) |
| Duration | 10 minutes (600,000ms) |
| Prerequisites | Iron Fists Lv2 |

**Max Spheres = SkillLv** (1-5). Each cast summons 1 sphere up to the cap.
Each sphere: +3 ATK (holy, ignores DEF). Sphere ATK is NOT affected by cards or weapon element.
Casting resets the 10-minute timer for ALL spheres.
Spheres persist through death but are lost on disconnect.

---

### Investigate / Occult Impaction (ID 1602)

| Property | Value |
|----------|-------|
| rAthena ID | 266 (MO_INVESTIGATE) |
| Type | Offensive, Physical |
| Max Level | 5 |
| Target | Single enemy |
| Range | 2 cells (melee) |
| Cast Time | 1000ms (interruptible) |
| After-Cast Delay | 500ms |
| Sphere Cost | 1 |
| Element | Always Neutral (ignores weapon element) |
| Hit Type | Always hits (ignores FLEE and Perfect Dodge) |
| Prerequisites | Summon Spirit Sphere Lv5 |

**Pre-Renewal Damage Formula:**
```
Damage = ATK * (1 + 0.75 * SkillLv) * (EnemyHardDEF + EnemyVIT) / 50
```

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 10 | 14 | 17 | 19 | 20 |
| ATK% | 175 | 250 | 325 | 400 | 475 |

**Critical mechanic:** DEF MULTIPLIES damage instead of reducing it. Higher target DEF = more damage.
If enemy has 0 DEF + 0 VIT, damage is 0.
Size penalty still applies. Card bonuses still apply.
Does NOT subtract DEF in the pipeline -- completely inverted formula.

---

### Triple Attack / Raging Trifecta Blow (ID 1603)

| Property | Value |
|----------|-------|
| rAthena ID | 263 (MO_TRIPLEATTACK) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Dodge Lv5 |

**Proc Chance Formula (Pre-Renewal):** `30 - SkillLv` %
**ATK% Formula:** `100 + 20 * SkillLv` %

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Proc% | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 |
| Total ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

**Mechanics:**
- 3 hits at total ATK% (each hit = ATK% / 3).
- Uses weapon element.
- Triggers on auto-attack (replaces the normal hit).
- Opens the combo window for Chain Combo on proc.
- No SP cost (passive).
- Counts as a single strike for card bonus purposes.
- If both Double Attack and Triple Attack exist, Double Attack takes priority.

---

### Finger Offensive / Throw Spirit Sphere (ID 1604)

| Property | Value |
|----------|-------|
| rAthena ID | 267 (MO_FINGEROFFENSIVE) |
| Type | Offensive, Physical, Ranged |
| Max Level | 5 |
| Target | Single enemy |
| Range | 9 cells |
| SP Cost | 10 (all levels, flat) |
| Element | Weapon element |
| Prerequisites | Investigate Lv3 |

**Sphere Cost:** Equal to skill level (1-5 spheres).
**Cast Time:** `(1 + spheres_consumed) * 1000` ms (variable, reduced by DEX).
**After-Cast Delay:** 500ms.

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% per sphere | 150 | 200 | 250 | 300 | 350 |
| Max Spheres | 1 | 2 | 3 | 4 | 5 |

**ATK% Formula:** `100 + 50 * SkillLv` per sphere.
Each sphere = 1 hit at full skill ATK%. Lv5 with 5 spheres = 5 hits at 350% = 1750% total.
If fewer spheres than skill level, uses all remaining spheres (cast time scales with actual spheres consumed).

---

### Asura Strike / Guillotine Fist (ID 1605)

| Property | Value |
|----------|-------|
| rAthena ID | 271 (MO_EXTREMITYFIST) |
| Type | Offensive, Physical |
| Max Level | 5 |
| Target | Single enemy |
| Range | 2 cells (melee) |
| Sphere Cost | All 5 spheres consumed |
| SP Cost | ALL remaining SP consumed (contributes to damage) |
| Element | Always Neutral (ignores weapon element) |
| Hit Type | Always hits (ignores FLEE). Bypasses DEF. |
| Required State | Fury (Critical Explosion) must be active |
| Prerequisites | Finger Offensive Lv3, Critical Explosion Lv3 |

**THE DAMAGE FORMULA (Pre-Renewal):**
```
Damage = (WeaponATK + StatusATK) * (8 + SP / 10) + 250 + (150 * SkillLv)
```

Then apply: `* CardEffects * ElementModifier(Neutral vs target)`

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Flat Bonus | +400 | +550 | +700 | +850 | +1000 |
| Cast Time(s) | 4.0 | 3.5 | 3.0 | 2.5 | 2.0 |
| After-Cast Delay(s) | 3.0 | 2.5 | 2.0 | 1.5 | 1.0 |

**Cast Time Formula:** `4500 - 500 * SkillLv` ms (before DEX reduction)
**ACD Formula:** `3500 - 500 * SkillLv` ms
**Flat Bonus Formula:** `250 + 150 * SkillLv`

**Special Mechanics:**

1. **SP Consumption:** ALL remaining SP consumed. Higher SP = more damage.
2. **DEF Bypass:** Ignores target DEF entirely (both hard and soft). Card-based reductions still apply.
3. **Always Hits:** Ignores FLEE and Perfect Dodge.
4. **Fury Requirement:** Critical Explosion must be active. Consumed on use.
5. **Sphere Requirement:** All 5 spheres consumed.
6. **SP Regen Lockout:** Cannot naturally regenerate SP for 5 minutes (300,000ms) after use. Items that restore SP still work. Spirits Recovery (sitting) bypasses this.
7. **Combo Usage:** When chained after Combo Finish, Asura Strike has NO cast time (instant). Needs 5 spheres before combo starts (CF consumes 1, so 4 remain).
8. **Blade Stop Usage:** During Root Lv5, Asura Strike has NO cast time.
9. **Mastery ATK Excluded:** Weapon mastery ATK NOT included in calculation.
10. **Element:** Always Neutral regardless of weapon element or endows.

**Damage Example (1000 SP, 200 ATK, Lv5):**
```
200 * (8 + 100) + 1000 = 200 * 108 + 1000 = 21600 + 1000 = 22,600
```

---

### Spirits Recovery / Spiritual Cadence (ID 1606)

| Property | Value |
|----------|-------|
| rAthena ID | 260 (MO_SPIRITSRECOVERY) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Blade Stop Lv2 |

**Recovery while sitting (per 10 seconds):**

| Level | HP Recovery | SP Recovery |
|-------|------------|------------|
| 1 | MaxHP * 0.2% + 4 | MaxSP * 0.2% + 2 |
| 2 | MaxHP * 0.4% + 8 | MaxSP * 0.4% + 4 |
| 3 | MaxHP * 0.6% + 12 | MaxSP * 0.6% + 6 |
| 4 | MaxHP * 0.8% + 16 | MaxSP * 0.8% + 8 |
| 5 | MaxHP * 1.0% + 20 | MaxSP * 1.0% + 10 |

**General Formula:**
```
HP = floor(MaxHP * SkillLv / 500) + 4 * SkillLv
SP = floor(MaxSP * SkillLv / 500) + 2 * SkillLv
```

**Key Bypasses:**
- Works while sitting even when over 50% weight limit (up to 89%).
- Does NOT work at 90%+ weight.
- Bypasses Fury's SP regen disable.
- Bypasses Asura Strike's 5-minute SP regen lockout.
- Recovery every 10s normally, every 20s when 50-89% overweight.

---

### Absorb Spirit Sphere (ID 1607)

| Property | Value |
|----------|-------|
| rAthena ID | 262 (MO_ABSORBSPIRITS) |
| Type | Active |
| Max Level | 1 |
| SP Cost | 5 |
| Cast Time | 2000ms |
| Range | 9 cells |
| Prerequisites | Summon Spirit Sphere Lv5 |

**On Self:** Consumes all own spirit spheres. Recovers 7 SP per sphere (pre-renewal).
**On Monster:** 10% chance to drain SP = monster.level * 2 (rAthena: 20% chance).
**On Other Player (PvP):** Absorbs target's spheres, caster recovers 7 SP per sphere.

---

### Dodge (ID 1608)

| Property | Value |
|----------|-------|
| rAthena ID | 265 (MO_DODGE) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Iron Fists Lv5, Summon Spirit Sphere Lv5 |

**FLEE Bonus Formula:** `floor(1.5 * SkillLv)`

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +FLEE | 1 | 3 | 4 | 6 | 7 | 9 | 10 | 12 | 13 | 15 |

---

### Blade Stop / Root (ID 1609)

| Property | Value |
|----------|-------|
| rAthena ID | 269 (MO_BLADESTOP) |
| Type | Active, counter stance |
| Max Level | 5 |
| SP Cost | 10 |
| Sphere Cost | 1 (consumed on activation regardless of catch) |
| Cast Time | 0 |
| Prerequisites | Dodge Lv5 |

**Catch Window Formula:** `300 + 200 * SkillLv` ms

| Level | Catch Window | Lock Duration | Skills Usable During Lock |
|-------|-------------|--------------|--------------------------|
| 1 | 500ms | 20s | None |
| 2 | 700ms | 30s | Finger Offensive |
| 3 | 900ms | 40s | + Investigate |
| 4 | 1100ms | 50s | + Chain Combo |
| 5 | 1300ms | 60s | + Asura Strike |

**Lock Duration Formula:** `10000 + 10000 * SkillLv` ms

**Mechanics:**
1. Activation enters "catching" stance for the window duration.
2. If an enemy melee auto-attack hits during the window, the attack is caught.
3. Both Monk and attacker are locked (cannot move, cannot auto-attack).
4. Only the Monk can act using skills determined by Blade Stop level.
5. Sphere consumed on activation regardless.
6. Cannot catch skills (only auto-attacks). Cannot catch ranged attacks.

---

### Chain Combo / Raging Quadruple Blow (ID 1610)

| Property | Value |
|----------|-------|
| rAthena ID | 272 (MO_CHAINCOMBO) |
| Type | Offensive, Physical (Combo skill) |
| Max Level | 5 |
| Target | Inherited from combo chain |
| Range | 2 cells (melee) |
| Cast Time | 0 (instant combo) |
| Number of Hits | 4 |
| Element | Weapon element |
| isCombo | true |
| Prerequisites | Triple Attack Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Total ATK% | 200 | 250 | 300 | 350 | 400 |
| SP | 11 | 12 | 13 | 14 | 15 |

**ATK% Formula:** `150 + 50 * SkillLv` (per pre-renewal sources: 200-400%)
**SP Formula:** `10 + SkillLv`

**Activation:** ONLY during combo window after Triple Attack proc, or during Blade Stop Lv4+ lock.
Target inherited from combo chain. Opens combo window for Combo Finish.

---

### Critical Explosion / Fury (ID 1611)

| Property | Value |
|----------|-------|
| rAthena ID | 270 (MO_EXPLOSIONSPIRITS) |
| Type | Active, Self Buff |
| Max Level | 5 |
| SP Cost | 15 (all levels) |
| Sphere Cost | 5 (all consumed) |
| Duration | 180 seconds (all levels) |
| Cast Time | 0 |
| Prerequisites | Absorb Spirit Sphere Lv1 |

**CRIT Bonus Formula:** `7.5 + 2.5 * SkillLv`

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| +CRIT | 10 | 12.5 | 15 | 17.5 | 20 |

**Fury Status Effects:**
1. +CRIT bonus per skill level.
2. Enables Asura Strike usage.
3. Disables natural SP regeneration while active.
4. Spirits Recovery (sitting) still works during Fury.
5. Snap costs no spheres during Fury.
6. Visual aura around character.

---

### Steel Body / Mental Strength (ID 1612)

| Property | Value |
|----------|-------|
| rAthena ID | 268 (MO_STEELBODY) |
| Type | Active, Self Buff |
| Max Level | 5 |
| SP Cost | 200 (all levels) |
| Sphere Cost | 5 (all consumed) |
| Cast Time | 5000ms (uninterruptible) |
| Prerequisites | Combo Finish Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 30 | 60 | 90 | 120 | 150 |

**Duration Formula:** `30 * SkillLv` seconds

**See [Steel Body Mechanics](#steel-body-mechanics) section for full details.**

---

### Combo Finish / Raging Thrust (ID 1613)

| Property | Value |
|----------|-------|
| rAthena ID | 273 (MO_COMBOFINISH) |
| Type | Offensive, Physical (Combo skill) |
| Max Level | 5 |
| Target | Inherited + 5x5 AoE splash |
| Range | 2 cells (melee) |
| Cast Time | 0 (instant combo) |
| Number of Hits | 1 |
| Sphere Cost | 1 |
| Element | Weapon element |
| isCombo | true |
| Prerequisites | Chain Combo Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 300 | 360 | 420 | 480 | 540 |
| SP | 11 | 12 | 13 | 14 | 15 |

**ATK% Formula:** `240 + 60 * SkillLv`
**SP Formula:** `10 + SkillLv`

**Activation:** ONLY during combo window after Chain Combo. Consumes 1 sphere.
5x5 AoE splash: enemies around primary target take same damage. Splash targets knocked back 5 cells (primary target is NOT knocked back).
Opens combo window for Asura Strike if Fury is active and spheres remain.

---

### Ki Translation (ID 1614) -- Quest Skill

| Property | Value |
|----------|-------|
| rAthena ID | 1015 (MO_KITRANSLATION) |
| Type | Active, Supportive |
| Max Level | 1 |
| SP Cost | 40 |
| Cast Time | 2000ms |
| After-Cast Delay | 1000ms |
| Sphere Cost | 1 (transferred to target) |
| Target | Party member |
| Range | 9 cells |

**Effect:** Transfers 1 spirit sphere from Monk to a party member. Target max 5 spheres.

---

### Ki Explosion (ID 1615) -- Quest Skill

| Property | Value |
|----------|-------|
| rAthena ID | 1016 (MO_KIEXPLOSION) |
| Type | Offensive, Physical, AoE |
| Max Level | 1 |
| SP Cost | 20 |
| HP Cost | 10 |
| Cast Time | 0 |
| After-Cast Delay | 2000ms |
| ATK% | 300 |
| AoE | 3x3 around target |
| Target | Single enemy (melee) |

**Primary target:** 300% ATK damage.
**Adjacent enemies:** Knocked back 5 cells, 70% stun chance for 2 seconds.
Primary target is NOT knocked back.

---

### Body Relocation / Snap (Monk/Champion)

| Property | Value |
|----------|-------|
| rAthena ID | 264 (MO_BODYRELOCATION) |
| Type | Active, Ground target |
| Max Level | 1 |
| SP Cost | 14 |
| Sphere Cost | 1 (free during Fury) |
| Range | 18 cells |
| Prerequisites | Asura Strike Lv3, Spirit Recovery Lv2, Steel Body Lv3 |
| Cooldown | 2 seconds after Asura Strike |

**Effect:** Instant teleport to target cell. Requires Fury state for sphere-free usage.
Some implementations restrict this to Champion (trans) class only.

---

## Healing System

### Core Heal Formula (Pre-Renewal)

```
HealAmount = floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
```

**Variables:**
- `BaseLv` = Character's base level (1-99)
- `INT` = Character's total INT stat (base + bonus)
- `SkillLv` = Heal skill level (1-10)

**Scaling:**
- `(BaseLv + INT) / 8` provides a slow scaling factor. At BaseLv 99 + INT 99 = 198, factor = 24.
- `(4 + 8 * SkillLv)` provides the per-level multiplier. At Lv10 = 84.
- Maximum heal at 99/99/Lv10: 24 * 84 = 2,016 HP (before modifiers).

**Heal Power Modifiers (multiplicative):**
1. Equipment Heal Power (`bHealPower`): % increase from equipment (e.g., Rod of Recovery +10%).
2. Card Heal Power: % increase from cards.
3. Heal Received modifier on target: % bonus from target's equipment/cards.
4. These stack multiplicatively: `HealAmount * (1 + equipBonus/100) * (1 + cardBonus/100) * (1 + targetReceiveBonus/100)`.

### Sanctuary Healing (Flat, Not Formula-Based)

Sanctuary uses **fixed heal values per tick**, NOT the Heal formula:

| Level | Heal/Tick |
|-------|-----------|
| 1-6 | 100 * SkillLv (100, 200, 300, 400, 500, 600) |
| 7-10 | 777 (capped) |

Equipment heal power bonuses DO apply to Sanctuary ticks.
Sanctuary is NOT affected by MATK.
1 tick per second for all players in the 5x5 AoE, up to a target limit.

### Heal vs Undead

When Heal targets an Undead element enemy:
- Damage = `floor(HealAmount / 2)` (50% of what would be healed)
- Modified by Holy vs Undead element level table (100%/125%/150%/175% for Undead Lv1-4)
- Ignores MDEF entirely
- Uses the standard Heal formula as base, then halves

---

## Buff System

### Blessing

**Three paths:**
1. **Friendly target:** +1-10 STR/DEX/INT. Duration 60-240s.
2. **Cursed/Stoned target:** Cures the status. Does NOT apply stat buff.
3. **Undead/Demon enemy:** Halves STR, DEX, INT. Boss immune.

### Increase AGI

- +3-12 AGI, +25% movement speed. Duration 60-240s.
- Costs 15 HP per cast (unique among buffs). Fails if HP < 16.
- Cancels Decrease AGI.

### Decrease AGI

- -3-12 AGI, -25% movement speed. Duration 30-120s (monsters).
- Success rate: `40 + 2*Lv + (BaseLv+INT)/5 - TargetMDEF` %. Boss immune.
- Cancels: Inc AGI, Adrenaline Rush, THQ, Spear Quicken, Cart Boost.

### Angelus

- +5-50% VIT-based soft DEF. Duration 30-300s. Party-wide.

### Impositio Manus

- +5-25 flat ATK (mastery-type, bypasses DEF). Duration 60s. Self/Ally.

### Suffragium

- -15/30/45% cast time on next spell. Duration 30/20/10s. Ally only (not self).
- Consumed on next spell cast.

### Aspersio

- Endows weapon with Holy element. Duration 60-180s. Self/Ally.
- Consumes 1 Holy Water. Cancels other endows.

### Gloria

- +30 LUK (flat). Duration 10-30s. Party-wide.

### Magnificat

- Doubles natural HP and SP regeneration. Duration 30-90s. Party-wide.

### Kyrie Eleison

- Creates damage barrier based on MaxHP%. Duration 120s.
- Blocks physical only (melee + ranged). Magic passes through.
- Breaks when hit limit or HP durability is depleted.

### Critical Explosion / Fury

- +10-20 CRIT. Duration 180s. Enables Asura Strike. Disables SP regen.
- Costs 5 spheres.

---

## Exorcism Skills

### Turn Undead

**Dual-mode skill targeting Undead element enemies:**

1. **Instant Kill (non-boss):**
   - Chance = `[(20 * Lv) + BaseLv + INT + LUK + {1 - HP/MaxHP} * 200] / 1000`
   - Capped at 70%.
   - Low HP targets are much easier to kill.

2. **Fail Damage (always applies to bosses):**
   - `(BaseLv + INT + Lv * 10) * 2` Holy piercing damage.
   - Ignores MDEF.

### Magnus Exorcismus

**Multi-wave ground AoE Holy damage vs Undead/Demon:**

- 7x7 cross-shaped AoE, 1-10 waves.
- 100% MATK per wave (130% vs Demon/Undead/Shadow targets).
- 3-second immunity between waves per monster (max 5 hits per target).
- 15s cast time (reduced by DEX), 1 Blue Gemstone.

### Aspersio

**Holy weapon endow:**
- Enchants weapon with Holy element.
- Effective against Shadow and Undead element enemies.
- 1 Holy Water consumed.

### Holy Light (Acolyte quest skill)

- 125% MATK Holy damage.
- Cancels Kyrie Eleison on target.

---

## Resurrection Mechanics

### Resurrection (ID 1004)

- Revives a dead player character at their current death position.
- HP restored: 10%/30%/50%/80% MaxHP per level.
- SP Cost: 60 (all levels).
- Cast Time: 6/4/2/0 seconds per level (Lv4 is instant).
- Consumes 1 Blue Gemstone.
- Cannot be used on self.
- Cannot be used on a player who has already self-respawned.

### Redemptio (ID 1018)

- Mass resurrects all dead party members within 15x15 cells.
- Restores 50% MaxHP to each resurrected member.
- Caster HP and SP drop to 1.
- Costs 1% of base EXP (reduced by 0.2% per member raised).
- 4-second uninterruptible cast.
- Quest skill.

---

## Safety Wall Mechanics

### Hit-Based Melee Blocking

Safety Wall creates a 1x1 cell barrier that blocks ALL melee physical attacks:

**What it blocks:**
- All regular auto-attacks at melee range
- Melee skills (Bash, Pierce, Asura Strike, etc.)
- Some ranged skills when used at melee range

**What passes through:**
- All magic damage
- Ranged physical attacks (arrows, thrown weapons)
- Status effects (stun from Hammer Fall still applies)
- Knockback effects (still applied)

**Durability System:**
1. **Hit count:** 2-11 hits blocked depending on level.
2. **HP durability:** `300 * SkillLv + 7000 * (1 + 0.1 * JobLv / 50) + 65 * INT + MaxSP`
3. Wall breaks when EITHER hits depleted OR HP depleted.
4. The hit that breaks it is **fully absorbed** (always blocks at least 1 hit).

**Interactions:**
- Cannot overlap with Pneuma or Magnetic Earth on same cell.
- Cannot stack multiple Safety Walls on same cell.
- Consumes 1 Blue Gemstone.
- Defense priority: Safety Wall checks BEFORE Kyrie Eleison.

---

## Spirit Sphere System

### Overview

Spirit Spheres are the Monk's core resource -- floating holy orbs that orbit the character.

### Properties

| Property | Value |
|----------|-------|
| Max Spheres | 5 (Monk), potentially 10 (Champion) |
| ATK Bonus | +3 per sphere (flat mastery ATK, ignores DEF) |
| Element | Holy (not affected by weapon element cards) |
| Duration | 10 minutes per summon batch |
| Persistence | Survives death, lost on disconnect |
| Summoning | 1 sphere per cast of Summon Spirit Sphere |

### ATK Application

Sphere ATK (+3 per sphere) is mastery-type:
- Added AFTER all multipliers (like Sword Mastery)
- NOT affected by element modifiers, size penalties, or card multipliers
- Added to `getPassiveSkillBonuses()` as `player.spiritSpheres * 3`

### Sphere Consumers

| Skill | Spheres Consumed |
|-------|-----------------|
| Investigate | 1 |
| Finger Offensive | SkillLv (1-5) |
| Asura Strike | All 5 |
| Critical Explosion (Fury) | All 5 |
| Steel Body | All 5 |
| Blade Stop | 1 |
| Combo Finish | 1 |
| Ki Translation | 1 (transferred) |
| Snap (Body Relocation) | 1 (free during Fury) |

### Absorb Spirit Sphere

- Self: absorbs own spheres, gains 7 SP per sphere (pre-renewal).
- Monster: 10-20% chance, gains SP = monster.level * 2.
- PvP: absorbs target player's spheres.

---

## Combo System

### Combo Chain

```
Auto-Attack --> Triple Attack (passive proc, 20-29% chance)
                    |
                    v
               Chain Combo (4 hits, 200-400% ATK, combo only)
                    |
                    v
               Combo Finish (1 hit, 300-540% ATK, AoE splash, 1 sphere)
                    |
                    v
               Asura Strike (massive damage, all SP + all spheres, Fury required)
```

### Combo Window Timing

**Formula:** `comboWindow = 1.3 - (AGI * 0.004) - (DEX * 0.002)` seconds

| AGI | DEX | Window |
|-----|-----|--------|
| 1 | 1 | 1.294s |
| 50 | 30 | 1.040s |
| 80 | 50 | 0.880s |
| 99 | 80 | 0.744s |
| 99 | 99 | 0.706s |

### Combo Rules

1. **Triple Attack -> Chain Combo:** CC only during TA combo window (or Blade Stop Lv4).
2. **Chain Combo -> Combo Finish:** CF only during CC combo window.
3. **Combo Finish -> Asura Strike:** Only if Fury active + spheres remain. Asura has NO cast time in combo.
4. All combo skills have 0 cast time, use weapon element, are physical damage, and cannot miss.
5. Target is inherited from initial auto-attack.

### Server-Side Combo State

```js
player.comboState = {
    active: false,
    lastSkillId: null,
    windowExpires: 0,
    targetId: null,
    isEnemy: true
};
```

### Combo Validation

For each combo skill:
1. Check `comboState.active === true`
2. Check `comboState.lastSkillId` matches expected predecessor
3. Check `Date.now() < comboState.windowExpires`
4. Check player has skill learned and enough SP/spheres
5. Execute, then set `comboState.lastSkillId` to current skill and reset window

---

## Steel Body Mechanics

### Effects When Active

| Effect | Value |
|--------|-------|
| Hard DEF override | 90 (replaces equipment DEF) |
| Hard MDEF override | 90 (replaces equipment MDEF) |
| ASPD reduction | -25% |
| Movement speed reduction | -25% |
| Active skills | BLOCKED (cannot use any active skills) |
| Passive skills | Still function (Triple Attack, Dodge, Iron Fists) |
| Items | Usable (potions, etc.) |
| Auto-cast skills | Card/equipment auto-casts still trigger |

### Duration

30/60/90/120/150 seconds at levels 1-5.

### Activation Cost

- 200 SP (all levels)
- 5 Spirit Spheres (all consumed)
- 5000ms cast time (uninterruptible)

### Damage Pipeline Integration

When a target has `steel_body` buff active:
```
Use overrideHardDEF (90) instead of equipment DEF
Use overrideHardMDEF (90) instead of equipment MDEF
```

This makes the character extremely tanky (90 hard DEF = 90% physical damage reduction, 90 hard MDEF = 90% magical damage reduction in pre-renewal).

### Stacking

Stacks with Assumptio (High Priest buff) in some implementations.

---

## Sitting / Meditation System

### Standard Sitting

- Player enters sitting state (`player.isSitting = true`).
- While sitting, cannot move, attack, or use skills.
- Natural HP/SP regeneration rate is doubled (2x normal regen).
- Standing up cancels sitting immediately.

### Spirits Recovery (Monk Passive)

Spirits Recovery enables special sitting regen for Monks:

1. **Works at 50-89% weight** (normal regen stops at 50% weight for other classes).
2. **Bypasses Fury SP regen disable** -- Monks can recover SP during Fury by sitting.
3. **Bypasses Asura SP regen lockout** -- after using Asura Strike, Monks can sit to recover SP despite the 5-minute lockout.
4. **Does NOT work at 90%+ weight.**

**Recovery per 10 seconds (while sitting):**
```
HP = floor(MaxHP * SkillLv / 500) + 4 * SkillLv
SP = floor(MaxSP * SkillLv / 500) + 2 * SkillLv
```

This is the primary way Asura Strike Monks recover SP for repeated casting.

---

## Skill Trees and Prerequisites

### Acolyte Skill Tree

```
Row 0:
  (0,0) Heal
  (0,1) Inc AGI [Heal 3]
  (0,2) Divine Protection
  (0,3) Demon Bane [Div Prot 3]

Row 1:
  (1,0) Cure [Heal 2]
  (1,1) Dec AGI [Inc AGI 1]
  (1,2) Blessing [Div Prot 5]
  (1,3) Signum Crucis [Demon Bane 3]

Row 2:
  (2,0) Ruwach
  (2,2) Angelus [Div Prot 3]

Row 3:
  (3,0) Teleport [Ruwach 1]

Row 4:
  (4,0) Warp Portal [Teleport 2]
  (4,2) Aqua Benedicta

Row 5:
  (5,0) Pneuma [Warp Portal 4]
  (5,2) Holy Light (quest skill)
```

### Priest Skill Tree

```
Row 0:
  (0,0) Sanctuary [Heal 1 (Aco)]
  (0,1) Kyrie Eleison [Angelus 2 (Aco)]
  (0,2) Resurrection [Heal 4, Inc SP Rec 4]
  (0,3) Mace Mastery

Row 1:
  (1,0) Magnificat
  (1,1) Gloria [Kyrie 4, Magnificat 3]
  (1,2) Lex Aeterna [Lex Divina 5]
  (1,3) Impositio Manus [Mace Mastery 3]

Row 2:
  (2,0) Magnus Exorcismus [Turn Undead 3, Lex Aeterna 1, Safety Wall 1]
  (2,1) Turn Undead [Resurrection 1, Lex Divina 1]
  (2,2) Suffragium [Impositio 2]
  (2,3) Aspersio [Aqua Benedicta 1 (Aco), Impositio 3]

Row 3:
  (3,0) Slow Poison
  (3,1) Status Recovery
  (3,2) Lex Divina [Ruwach 1 (Aco)]
  (3,3) B.S. Sacramenti [Aspersio 3, Gloria 3]

Row 4:
  (4,0) Increase SP Recovery
  (4,1) Safety Wall (Priest) [Inc SP Recovery 3]
  (4,2) Redemptio (quest skill)
```

### Monk Skill Tree

```
Row 0:
  (0,0) Iron Fists [Div Prot 10, Demon Bane 10]
  (0,1) Summon Spirit Sphere [Iron Fists 2]
  (0,2) Spirits Recovery [Blade Stop 2]
  (0,3) Absorb Spirit Sphere [SSS 5]

Row 1:
  (1,0) Investigate [SSS 5]
  (1,1) Triple Attack [Dodge 5]
  (1,2) Dodge [Iron Fists 5, SSS 5]
  (1,3) Critical Explosion [Absorb 1]

Row 2:
  (2,0) Finger Offensive [Investigate 3]
  (2,1) Chain Combo [Triple Attack 5]
  (2,2) Blade Stop [Dodge 5]

Row 3:
  (3,0) Asura Strike [Finger Off 3, Crit Exp 3]
  (3,1) Combo Finish [Chain Combo 3]
  (3,2) Steel Body [Combo Finish 3]

Row 4:
  (4,0) Ki Translation (quest skill)
  (4,1) Ki Explosion (quest skill)
```

### Key Prerequisite Chains

**Heal Chain:** Heal -> Sanctuary -> (no further Priest prereqs)
**Defense Chain:** Angelus -> Kyrie Eleison -> Gloria (+ Magnificat 3)
**Exorcism Chain:** Resurrection -> Turn Undead -> Magnus Exorcismus (+ Lex Aeterna 1 + Safety Wall 1)
**Debuff Chain:** Ruwach -> Lex Divina -> Lex Aeterna -> Magnus Exorcismus
**ATK Support Chain:** Mace Mastery -> Impositio Manus -> Suffragium / Aspersio -> B.S. Sacramenti
**SP/Defense Chain:** Inc SP Recovery -> Safety Wall
**Monk Combo Chain:** Iron Fists -> SSS -> Dodge -> Triple Attack -> Chain Combo -> Combo Finish -> (Steel Body | Asura Strike)
**Sphere Chain:** Iron Fists -> SSS -> Absorb -> Critical Explosion -> Asura Strike

---

## Implementation Checklist

### Acolyte Skills (15 total)

| ID | Name | Handler Status | Priority |
|----|------|---------------|----------|
| 400 | Heal | IMPLEMENTED | -- |
| 401 | Divine Protection | IMPLEMENTED (passive) | LOW (add BaseLv scaling) |
| 402 | Blessing | PARTIAL (missing Undead/Demon debuff, Curse/Stone cure) | HIGH |
| 403 | Increase AGI | PARTIAL (missing 15 HP cost, min-16 check, ACD) | HIGH |
| 404 | Decrease AGI | PARTIAL (missing success rate, boss immunity) | HIGH |
| 405 | Cure | IMPLEMENTED | -- |
| 406 | Angelus | PARTIAL (self-only, needs party-wide) | MEDIUM |
| 407 | Signum Crucis | PARTIAL (missing success rate, wrong AoE/duration) | MEDIUM |
| 408 | Ruwach | IMPLEMENTED (detection deferred) | LOW |
| 409 | Teleport | IMPLEMENTED | -- |
| 410 | Warp Portal | PARTIAL (memo system implemented, gemstone done) | LOW |
| 411 | Pneuma | PARTIAL (no overlap check, no skill blocking) | MEDIUM |
| 412 | Aqua Benedicta | SIMPLIFIED (no item creation) | LOW |
| 413 | Demon Bane | IMPLEMENTED (passive) | LOW (add BaseLv scaling) |
| 414 | Holy Light | NEEDS HANDLER | HIGH |

### Priest Skills (19 total)

| ID | Name | Handler Status | Priority |
|----|------|---------------|----------|
| 1000 | Sanctuary | NEEDS HANDLER | HIGH |
| 1001 | Kyrie Eleison | NEEDS HANDLER | HIGH |
| 1002 | Magnificat | NEEDS HANDLER | MEDIUM |
| 1003 | Gloria | NEEDS HANDLER | MEDIUM |
| 1004 | Resurrection | NEEDS HANDLER | MEDIUM |
| 1005 | Magnus Exorcismus | NEEDS HANDLER | MEDIUM |
| 1006 | Turn Undead | NEEDS HANDLER | MEDIUM |
| 1007 | Lex Aeterna | NEEDS HANDLER | HIGH |
| 1008 | Mace Mastery | NEEDS PASSIVE | HIGH |
| 1009 | Impositio Manus | NEEDS HANDLER | HIGH |
| 1010 | Suffragium | NEEDS HANDLER | MEDIUM |
| 1011 | Aspersio | NEEDS HANDLER | HIGH |
| 1012 | B.S. Sacramenti | NEEDS HANDLER | LOW |
| 1013 | Slow Poison | NEEDS HANDLER | LOW |
| 1014 | Status Recovery | NEEDS HANDLER | MEDIUM |
| 1015 | Lex Divina | NEEDS HANDLER | MEDIUM |
| 1016 | Inc SP Recovery | NEEDS PASSIVE | HIGH |
| 1017 | Safety Wall (Priest) | NEEDS HANDLER | MEDIUM |
| 1018 | Redemptio | DEFERRED (party) | LOW |

### Monk Skills (16 total)

| ID | Name | Handler Status | Priority |
|----|------|---------------|----------|
| 1600 | Iron Fists | IMPLEMENTED (passive) | -- |
| 1601 | Summon Spirit Sphere | IMPLEMENTED | -- |
| 1602 | Investigate | IMPLEMENTED | -- |
| 1603 | Triple Attack | IMPLEMENTED | -- |
| 1604 | Finger Offensive | IMPLEMENTED | -- |
| 1605 | Asura Strike | IMPLEMENTED | -- |
| 1606 | Spirits Recovery | IMPLEMENTED | -- |
| 1607 | Absorb Spirit Sphere | IMPLEMENTED | -- |
| 1608 | Dodge | IMPLEMENTED (passive) | -- |
| 1609 | Blade Stop | IMPLEMENTED | -- |
| 1610 | Chain Combo | IMPLEMENTED | -- |
| 1611 | Critical Explosion | IMPLEMENTED | -- |
| 1612 | Steel Body | IMPLEMENTED | -- |
| 1613 | Combo Finish | IMPLEMENTED | -- |
| 1614 | Ki Translation | IMPLEMENTED | -- |
| 1615 | Ki Explosion | IMPLEMENTED | -- |

### New Systems Required

| System | Needed For | Priority |
|--------|-----------|----------|
| Ground Heal Zone | Sanctuary | HIGH |
| Damage Absorb Barrier | Kyrie Eleison | HIGH |
| Multi-Wave Ground AoE | Magnus Exorcismus | MEDIUM |
| One-Hit Consumable Debuff | Lex Aeterna | HIGH |
| Element Endow Buff | Aspersio | HIGH |
| Cast Time Reduction Buff | Suffragium | MEDIUM |
| Regen Multiplier Buff | Magnificat | MEDIUM |
| Player Resurrection | Resurrection | MEDIUM |
| Melee Block Ground Effect | Safety Wall | MEDIUM |
| Instant Kill Mechanic | Turn Undead | MEDIUM |

---

## Gap Analysis

### Critical Gaps (Must Fix)

1. **Blessing Undead/Demon path missing** -- enemy targeting branch does not exist. Blessing only buffs allies.
2. **Increase AGI missing HP cost** -- should deduct 15 HP per cast, fail if HP < 16.
3. **Decrease AGI missing success rate** -- always succeeds, should have 42-60% base + formula with MDEF resistance. Boss immunity missing.
4. **Holy Light has no handler** -- quest skill with no execution logic.
5. **All 19 Priest skill handlers missing** -- definition-only, no execution.

### Important Gaps (Should Fix)

6. **Sanctuary heal values wrong for Lv7+** -- uses `100*Lv` instead of capping at 777.
7. **Sanctuary duration formula wrong** -- `4+Lv` seconds instead of `1+3*Lv` seconds.
8. **Kyrie Eleison SP cost formula wrong** -- uses `20+2*Lv` instead of stepped formula.
9. **Magnificat duration wrong** -- `30+10*Lv` instead of `15+15*Lv`.
10. **Lex Divina SP/duration arrays wrong** -- needs manual arrays for non-linear progression.
11. **Magnus Exorcismus effectValue wrong** -- scales when it should be flat 100% per wave.

### Data Corrections Needed

12. **Triple Attack prereqs wrong** -- should be Dodge Lv5, not Iron Fists Lv5.
13. **Spirits Recovery prereqs wrong** -- should be Blade Stop Lv2, not SSS Lv2.
14. **Critical Explosion prereqs wrong** -- should be Absorb Lv1, not SSS 5 + Iron Fists 5.
15. **Steel Body prereqs wrong** -- should be Combo Finish Lv3, not CE 3 + BS 3.
16. **Investigate castTime wrong** -- should be 1000, not 500.
17. **Finger Offensive SP should be flat 10**, not scaling.
18. **Asura Strike effectValue wrong** -- flat bonus should be 400-1000, not 250-850.
19. **Absorb Spirit Sphere gives 7 SP per sphere** (pre-renewal), not 10.
20. **Dodge FLEE values wrong** -- should use `floor(1.5*Lv)`, not `1+Lv`.

### Deferred (Lower Priority)

21. Party-wide buffs (Angelus, Magnificat, Gloria, B.S. Sacramenti) -- needs party system.
22. Ruwach hidden detection -- needs Hiding/Cloaking status system.
23. Aqua Benedicta item creation -- needs water cell detection.
24. Divine Protection/Demon Bane base level scaling -- minor (~4-5 bonus at 99).
25. Redemptio -- needs party system for mass resurrect.
26. Snap/Body Relocation -- can be deferred to Champion class.

---

## Sources

- [iRO Wiki Classic - Heal](https://irowiki.org/classic/Heal)
- [iRO Wiki Classic - Monk](https://irowiki.org/classic/Monk)
- [iRO Wiki - Sanctuary](https://irowiki.org/wiki/Sanctuary)
- [iRO Wiki - Magnus Exorcismus](https://irowiki.org/wiki/Magnus_Exorcismus)
- [iRO Wiki - Guillotine Fist](https://irowiki.org/wiki/Guillotine_Fist)
- [iRO Wiki - Turn Undead](https://irowiki.org/wiki/Turn_Undead)
- [iRO Wiki - Safety Wall](https://irowiki.org/wiki/Safety_Wall)
- [iRO Wiki - Kyrie Eleison](https://irowiki.org/wiki/Kyrie_Eleison)
- [RateMyServer - Priest Skills](https://ratemyserver.net/index.php?page=skill_db&jid=8)
- [RateMyServer - Monk Skills](https://ratemyserver.net/index.php?page=skill_db&jid=15)
- [RateMyServer - Asura Strike](https://ratemyserver.net/index.php?page=skill_db&skid=271)
- [rAthena Pre-renewal Skill DB](https://github.com/flaviojs/rathena-commits/blob/master/db/pre-re/skill_db.txt)
- [rAthena Pre-renewal Database](https://pre.pservero.com/job/Priest/)
- [Ragnarok Wiki - Asura Strike](https://ragnarok.fandom.com/wiki/Asura_Strike)
- [TalonRO Wiki - Turn Undead](https://wiki.talonro.com/Priest_-_Turn_Undead)
- [GameFAQs - Combo Monk Guide](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/37726)
- [GameFAQs - Monk Guide by CalxZinra](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/36497)
