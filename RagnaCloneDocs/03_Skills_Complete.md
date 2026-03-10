# Ragnarok Online Classic (Pre-Renewal) — Complete Skill Reference

> Sources: iRO Wiki Classic, RateMyServer Skill DB, rAthena pre-re skill_db.yml
> All data is Pre-Renewal unless noted. Skill IDs use the project's internal numbering (see `server/src/ro_skill_data.js`). Official rAthena IDs shown in parentheses where different.

---

## Table of Contents

1. [Skill Mechanics](#skill-mechanics)
2. [Novice](#novice)
3. [Swordsman](#swordsman)
4. [Mage](#mage)
5. [Archer](#archer)
6. [Thief](#thief)
7. [Merchant](#merchant)
8. [Acolyte](#acolyte)
9. [Knight](#knight)
10. [Crusader](#crusader)
11. [Wizard](#wizard)
12. [Sage](#sage)
13. [Hunter](#hunter)
14. [Bard](#bard)
15. [Dancer](#dancer)
16. [Assassin](#assassin)
17. [Rogue](#rogue)
18. [Blacksmith](#blacksmith)
19. [Alchemist](#alchemist)
20. [Priest](#priest)
21. [Monk](#monk)
22. [Lord Knight](#lord-knight)
23. [Paladin](#paladin)
24. [High Wizard](#high-wizard)
25. [Scholar (Professor)](#scholar-professor)
26. [Sniper](#sniper)
27. [Clown (Minstrel)](#clown-minstrel)
28. [Gypsy](#gypsy)
29. [Assassin Cross](#assassin-cross)
30. [Stalker](#stalker)
31. [Whitesmith (Mastersmith)](#whitesmith-mastersmith)
32. [Creator (Biochemist)](#creator-biochemist)
33. [High Priest](#high-priest)
34. [Champion](#champion)
35. [Implementation Notes](#implementation-notes)

---

## Skill Mechanics

### Cast Time (Pre-Renewal)

```
Final Cast Time = Base Cast Time * (1 - DEX / 150) * (1 - Item Cast Reduction%)
```

- At **150 DEX**, all variable cast time becomes **0** (instant cast).
- At 75 DEX, cast time is halved.
- There is NO fixed cast time in pre-renewal. All cast time is variable.
- Suffragium reduces cast time by 15/30/45%.
- Items like Phen Card (cast not interrupted), Drops Card (-cast time%) also apply multiplicatively.

### After-Cast Delay (ACD)

- Global cooldown after a skill finishes casting. No other skills can be used during ACD.
- Reduced by Bragi's Poem (A Poem of Bragi) performance skill.
- NOT reduced by DEX.
- Separate from skill-specific cooldowns.

### Skill Interruption

- By default, taking damage while casting interrupts the cast.
- Endure, Phen Card, and similar effects prevent interruption.
- Moving always cancels casting.

### Skill Target Types

| Type | Description |
|------|-------------|
| `self` | Affects caster only |
| `single` | Requires clicking an enemy/ally target |
| `ground` | Requires clicking a ground cell |
| `aoe` | Centered on caster, hits all in range |
| `none` | Passive, always active |

### Skill Tree / Prerequisites

- Skills require prerequisite skills at specific levels before they can be learned.
- Skill points are earned 1 per job level (49 total for 1st class, 49 for 2nd class).
- Transcendent classes get the same skill points but can also learn new trans-only skills.

### Damage Types

| Type | Uses | Defense |
|------|------|---------|
| Physical (melee) | ATK, STR, weapon | Hard DEF + Soft DEF (VIT) |
| Physical (ranged) | ATK, DEX, weapon | Hard DEF + Soft DEF (VIT) |
| Magical | MATK, INT | Hard MDEF + Soft MDEF (INT) |

### Element Table

Damage is multiplied by elemental modifiers. Key interactions:
- Water strong vs Fire, weak vs Wind
- Fire strong vs Earth, weak vs Water
- Wind strong vs Water, weak vs Earth
- Earth strong vs Wind, weak vs Fire
- Holy strong vs Shadow/Undead
- Ghost hits Ghost for 0 (except Ghost vs Ghost Lv1 = 25%)
- Neutral has no particular weakness

---

## Novice

### Basic Skill
| Field | Value |
|-------|-------|
| ID | 1 (rA: 1) |
| Type | Passive |
| Max Level | 9 |
| Target | None |
| Element | Neutral |
| SP Cost | 0 |
| Cast Time | 0 |
| Description | Enables basic commands (trade, chat, party, Kafra). Lv9 required for 1st class change. |

| Level | Effect |
|-------|--------|
| 1 | Enables Trade |
| 3 | Enables Emotes |
| 5 | Enables Chat Room |
| 6 | Enables Party |
| 7 | Enables Kafra Storage |
| 9 | Enables Job Change |

### First Aid
| Field | Value |
|-------|-------|
| ID | 2 (rA: 142) |
| Type | Active |
| Max Level | 1 |
| Target | Self |
| SP Cost | 3 |
| Cast Time | 0 |
| Cooldown | 0 |
| Description | Restore 5 HP. Quest skill. |

### Play Dead (Trick Dead)
| Field | Value |
|-------|-------|
| ID | 3 (rA: 143) |
| Type | Toggle |
| Max Level | 1 |
| Target | Self |
| SP Cost | 0 |
| Cast Time | 0 |
| Description | Feign death. Monsters deaggro. Cannot move/attack/use items while active. Quest skill. |

---

## Swordsman

### Sword Mastery
| Field | Value |
|-------|-------|
| ID | 100 (rA: 2) |
| Type | Passive |
| Max Level | 10 |
| Target | None |
| Prerequisites | None |
| Description | +ATK with 1H Swords and Daggers (ignores DEF). |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

### Two-Handed Sword Mastery
| Field | Value |
|-------|-------|
| ID | 101 (rA: 3) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Sword Mastery Lv1 |
| Description | +ATK with 2H Swords (ignores DEF). |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

### Increase HP Recovery
| Field | Value |
|-------|-------|
| ID | 102 (rA: 4) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |
| Description | Increases natural HP regen while stationary. Also increases healing item effectiveness. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +HP/10s | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| Item +% | 10 | 20 | 30 | 40 | 50 | 60 | 70 | 80 | 90 | 100 |

### Bash
| Field | Value |
|-------|-------|
| ID | 103 (rA: 5) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Element | Neutral |
| Prerequisites | None |
| Cast Time | 0 |
| After-Cast Delay | ASPD-based |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 130 | 160 | 190 | 210 | 240 | 270 | 300 | 330 | 360 | 400 |
| SP | 8 | 8 | 8 | 8 | 8 | 15 | 15 | 15 | 15 | 15 |
| HIT+ | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| Stun% (Fatal Blow) | - | - | - | - | - | 5 | 10 | 15 | 20 | 25 |

- Fatal Blow stun chance requires quest skill (unlocked at Lv6+).

### Provoke
| Field | Value |
|-------|-------|
| ID | 104 (rA: 6) |
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Neutral |
| Prerequisites | None |
| Cast Time | 0 |
| Duration | 30s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
| Target ATK+ | 5% | 8% | 11% | 14% | 17% | 20% | 23% | 26% | 29% | 32% |
| Target DEF- | 10% | 15% | 20% | 25% | 30% | 35% | 40% | 45% | 50% | 55% |
| Success% | 53 | 56 | 59 | 62 | 65 | 68 | 71 | 74 | 77 | 80 |

- Fails vs Boss/Undead monsters.

### Magnum Break
| Field | Value |
|-------|-------|
| ID | 105 (rA: 7) |
| Type | Offensive |
| Max Level | 10 |
| Target | AoE (self-centered 5x5) |
| Range | Melee |
| Element | Fire |
| Prerequisites | Bash Lv5 |
| Cast Time | 0 |
| After-Cast Delay | 2s |
| HP Cost | 20 at Lv1, -0.5/level (formula varies) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |
| SP | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 30 |

- Knockback 2 cells.
- Grants +20% Fire property damage to normal attacks for 10 seconds after cast.

### Endure
| Field | Value |
|-------|-------|
| ID | 106 (rA: 8) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Provoke Lv5 |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| Cooldown | 10s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration(s) | 10 | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 |
| +MDEF | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Hits | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 |

- Prevents flinch (movement/attack delay on hit) for up to 7 hits or until duration expires.

### Quest Skills

**Moving HP Recovery** (rA: 144) — Passive. Allows HP regen while walking (normally only regen while stationary). Job Lv25.

**Fatal Blow** (rA: 145) — Passive. Adds stun chance to Bash Lv6+. See Bash table. Job Lv30.

**Auto Berserk** (rA: 146) — Toggle. SP: 1. When HP drops below 25%, auto-activates Provoke Lv10 on self (+32% ATK, -55% DEF). Job Lv35.

---

## Mage

### Increase SP Recovery
| Field | Value |
|-------|-------|
| ID | 204 (rA: 9) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |
| Description | Increases natural SP regen while stationary. Also increases SP item effectiveness. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +SP/10s | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

### Sight
| Field | Value |
|-------|-------|
| ID | 205 (rA: 10) |
| Type | Active |
| Max Level | 1 |
| Target | Self |
| SP Cost | 10 |
| Duration | 10s |
| Description | Reveals hidden enemies in 7x7 area around caster. Fire element. |

### Cold Bolt
| Field | Value |
|-------|-------|
| ID | 200 (rA: 14) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells (magic) |
| Element | Water |
| Prerequisites | None |
| Formula | (Number of Bolts) x 1.0 MATK per bolt |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Bolts | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| SP | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Cast(s) | 0.7 | 1.4 | 2.1 | 2.8 | 3.5 | 4.2 | 4.9 | 5.6 | 6.3 | 7.0 |
| ACD(s) | 1.0 | 1.2 | 1.4 | 1.6 | 1.8 | 2.0 | 2.2 | 2.4 | 2.6 | 2.8 |

- Cast Time formula: `Level * 0.7s`
- After-Cast Delay formula: `0.8 + Level * 0.2s`
- All bolts hit simultaneously as one damage bundle.

### Fire Bolt
| Field | Value |
|-------|-------|
| ID | 201 (rA: 19) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Fire |
| Prerequisites | None |
| Formula | (Number of Bolts) x 1.0 MATK per bolt |

SP, cast time, ACD, and bolt count are **identical** to Cold Bolt. Only element differs.

### Lightning Bolt
| Field | Value |
|-------|-------|
| ID | 202 (rA: 20) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Wind |
| Prerequisites | None |
| Formula | (Number of Bolts) x 1.0 MATK per bolt |

SP, cast time, ACD, and bolt count are **identical** to Cold Bolt. Only element differs.

### Napalm Beat
| Field | Value |
|-------|-------|
| ID | 203 (rA: 11) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single (3x3 splash) |
| Range | 9 cells |
| Element | Ghost |
| Prerequisites | None |
| AoE | 3x3 around target |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% | 80 | 90 | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 |
| SP | 9 | 9 | 9 | 12 | 12 | 12 | 15 | 15 | 15 | 18 |
| Cast(s) | 1.0 | ~0.9 | ~0.9 | ~0.8 | ~0.8 | ~0.7 | ~0.7 | ~0.6 | ~0.6 | 0.5 |
| ACD(s) | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |

- Single hit regardless of level. Splash damage split among all targets hit.

### Soul Strike
| Field | Value |
|-------|-------|
| ID | 210 (rA: 13) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Ghost |
| Prerequisites | Napalm Beat Lv4 |
| Cast Time | 0.5s (all levels) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Hits | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 |
| SP | 18 | 14 | 24 | 20 | 30 | 26 | 36 | 32 | 42 | 38 |
| ACD(s) | 1.2 | 1.4 | 1.6 | 1.8 | 2.0 | 2.2 | 2.4 | 2.6 | 2.8 | 2.7 |
| Undead+% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

- Each hit deals 1x MATK. Hit formula: `floor((level + 1) / 2)`
- Bonus damage vs Undead element targets.

### Fire Ball
| Field | Value |
|-------|-------|
| ID | 207 (rA: 17) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single (5x5 splash) |
| Range | 9 cells |
| Element | Fire |
| Prerequisites | Fire Bolt Lv4 |
| AoE | 5x5 around target |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% | 80 | 90 | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 |
| SP | 25 | 25 | 25 | 25 | 25 | 25 | 25 | 25 | 25 | 25 |
| Cast(s) | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| ACD(s) | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |

### Frost Diver
| Field | Value |
|-------|-------|
| ID | 208 (rA: 15) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Water |
| Prerequisites | Cold Bolt Lv5 |
| Cast Time | 0.8s |
| ACD | 1.5s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 | 200 |
| SP | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
| Freeze% | 38 | 41 | 44 | 47 | 50 | 53 | 56 | 59 | 62 | 65 |
| Freeze(s) | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

- Freeze fails vs Boss and Undead element monsters (damage still applies).
- Target MDEF reduces freeze chance and duration.

### Stone Curse
| Field | Value |
|-------|-------|
| ID | 206 (rA: 16) |
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 2 cells |
| Element | Earth |
| Prerequisites | None |
| Cast Time | 1s |
| Catalyst | 1 Red Gemstone |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |
| Success% | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 |

- Lv1-5: Red Gemstone consumed regardless of success. Lv6-10: only consumed on success.
- Stone status drains 1% HP every 5s until 25% HP remains.
- Fails vs Boss and Undead element.

### Fire Wall
| Field | Value |
|-------|-------|
| ID | 209 (rA: 18) |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Range | 9 cells |
| Element | Fire |
| Prerequisites | Fire Bolt Lv4, Fire Ball Lv5, Sight Lv1 |
| AoE | 1x3 cells |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| Cast(s) | 2.0 | 1.85 | 1.7 | 1.55 | 1.4 | 1.25 | 1.1 | 0.95 | 0.8 | 0.65 |
| Hits | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Duration(s) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |

- Each hit deals 50% MATK.
- Knocks enemies back on contact.
- Hit formula: `2 + Level`

### Safety Wall
| Field | Value |
|-------|-------|
| ID | 211 (rA: 12) |
| Type | Supportive |
| Max Level | 10 |
| Target | Ground (1x1) |
| Range | 9 cells |
| Element | Neutral |
| Prerequisites | Napalm Beat Lv7, Soul Strike Lv5 |
| Catalyst | 1 Blue Gemstone |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 30 | 30 | 30 | 35 | 35 | 35 | 40 | 40 | 40 | 40 |
| Cast(s) | 4.0 | 3.5 | 3.0 | 2.5 | 2.0 | 1.5 | 1.0 | 1.0 | 1.0 | 1.0 |
| Hits Blocked | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
| Duration(s) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

- Blocks melee physical attacks only.

### Thunderstorm
| Field | Value |
|-------|-------|
| ID | 212 (rA: 21) |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Range | 9 cells |
| Element | Wind |
| Prerequisites | Lightning Bolt Lv4 |
| AoE | 5x5 |
| ACD | 2s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Hits | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| SP | 29 | 34 | 39 | 44 | 49 | 54 | 59 | 64 | 69 | 74 |
| Cast(s) | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |

- Each hit deals 80% MATK.
- SP formula: `24 + Level * 5`

### Energy Coat (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 157 |
| Type | Supportive |
| Max Level | 1 |
| Target | Self |
| SP Cost | 30 |
| Description | Reduces physical damage at the cost of SP. Damage reduction scales with current SP%. |

| SP Remaining | Damage Reduction | SP Drain per Hit |
|-------------|-----------------|-----------------|
| >80% | 30% | 3% SP |
| 61-80% | 24% | 2.5% SP |
| 41-60% | 18% | 2% SP |
| 21-40% | 12% | 1.5% SP |
| 1-20% | 6% | 1% SP |

---

## Archer

### Owl's Eye
| Field | Value |
|-------|-------|
| ID | 300 (rA: 43) |
| Type | Passive |
| Max Level | 10 |
| Description | +1 DEX per level (+10 DEX at Lv10). |

### Vulture's Eye
| Field | Value |
|-------|-------|
| ID | 301 (rA: 44) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Owl's Eye Lv3 |
| Description | +1 bow range per level, +1 HIT per level. |

### Improve Concentration
| Field | Value |
|-------|-------|
| ID | 302 (rA: 45) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Vulture's Eye Lv1 |
| Duration | 60s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 65 | 70 |
| AGI/DEX +% | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

- Also reveals hidden enemies in 3x3 area on cast.

### Double Strafe
| Field | Value |
|-------|-------|
| ID | 303 (rA: 46) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Bow range (+Vulture's Eye) |
| Element | Weapon element |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| Prerequisites | None |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% per hit | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 |

- Always 2 hits. Total ATK% = value x 2.
- Requires bow and arrows.

### Arrow Shower
| Field | Value |
|-------|-------|
| ID | 304 (rA: 47) |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground (3x3 AoE) |
| Range | Bow range |
| Element | Weapon element |
| SP Cost | 15 (all levels) |
| Prerequisites | Double Strafe Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 80 | 85 | 90 | 95 | 100 | 105 | 110 | 115 | 120 | 125 |

- Knocks enemies back 2 cells.
- Consumes 1 arrow.

### Arrow Crafting (Quest Skill)
| Field | Value |
|-------|-------|
| ID | 305 (rA: 147) |
| Type | Active |
| Target | Self |
| SP Cost | 10 |
| Description | Convert items into arrows. Job Lv30. |

### Arrow Repel (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 148 |
| Type | Offensive |
| Target | Single Enemy |
| SP Cost | 15 |
| Cast Time | 1.5s |
| Damage | 150% ATK. Knockback 6 cells. Job Lv35. |

---

## Thief

### Double Attack
| Field | Value |
|-------|-------|
| ID | 500 (rA: 48) |
| Type | Passive |
| Max Level | 10 |
| Description | Chance to hit twice with daggers on normal attacks. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Chance% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

### Improve Dodge
| Field | Value |
|-------|-------|
| ID | 501 (rA: 49) |
| Type | Passive |
| Max Level | 10 |
| Description | +3 FLEE per level. Lv10 also adds +1% move speed. |

### Steal
| Field | Value |
|-------|-------|
| ID | 502 (rA: 50) |
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| SP Cost | 10 |
| Description | Attempt to steal an item from a monster. Success based on DEX, level diff, monster stats. |

### Hiding
| Field | Value |
|-------|-------|
| ID | 503 (rA: 51) |
| Type | Toggle |
| Max Level | 10 |
| Target | Self |
| SP Cost | 10 |
| Prerequisites | Steal Lv5 |
| Description | Hide from enemies. Drains SP over time. Detected by Boss, Insect, Demon types, and detection skills. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration(s) | 30 | 36 | 42 | 48 | 54 | 60 | 66 | 72 | 78 | 84 |

### Envenom
| Field | Value |
|-------|-------|
| ID | 504 (rA: 52) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Element | Poison |
| SP Cost | 12 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 15 | 30 | 45 | 60 | 75 | 90 | 105 | 120 | 135 | 150 |

- Adds flat bonus damage on top of normal attack. 15% chance to poison target.

### Detoxify
| Field | Value |
|-------|-------|
| ID | 505 (rA: 53) |
| Type | Supportive |
| Max Level | 1 |
| Target | Single (self or ally) |
| SP Cost | 10 |
| Prerequisites | Envenom Lv3 |
| Description | Cure poison status. |

### Quest Skills

**Sprinkle Sand** (rA: 149) — Offensive. 150% ATK, chance to Blind. SP: 9. Job Lv25.

**Back Slide** (rA: 150) — Active. Instantly move 5 cells backward. SP: 7. Job Lv35.

**Pick Stone** (rA: 151) — Active. Pick up a stone for throwing. SP: 2. Job Lv15.

**Throw Stone** (rA: 152) — Offensive. 50 fixed damage + Stun chance. Range 7 cells. SP: 2. Job Lv20.

---

## Merchant

### Enlarge Weight Limit
| Field | Value |
|-------|-------|
| ID | 600 (rA: 36) |
| Type | Passive |
| Max Level | 10 |
| Description | +200 max weight per level (+2000 at Lv10). |

### Discount
| Field | Value |
|-------|-------|
| ID | 601 (rA: 37) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Enlarge Weight Limit Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| NPC Buy -% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |

### Overcharge
| Field | Value |
|-------|-------|
| ID | 602 (rA: 38) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Discount Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| NPC Sell +% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |

### Pushcart
| Field | Value |
|-------|-------|
| ID | rA: 39 |
| Type | Passive |
| Max Level | 10 |
| Description | Allows use of a cart (+8000 weight, 100 item slots). Higher levels reduce speed penalty. |

### Item Appraisal
| Field | Value |
|-------|-------|
| ID | rA: 40 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 10 |
| Description | Identify unidentified items without Magnifier. |

### Vending
| Field | Value |
|-------|-------|
| ID | rA: 41 |
| Type | Active |
| Max Level | 10 |
| SP Cost | 30 |
| Prerequisites | Pushcart Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Slots | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

### Mammonite
| Field | Value |
|-------|-------|
| ID | 603 (rA: 42) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Element | Neutral |
| SP Cost | 5 (all levels) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 150 | 200 | 250 | 300 | 350 | 400 | 450 | 500 | 550 | 600 |
| Zeny Cost | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | 1000 |

### Quest Skills

**Cart Revolution** (rA: 153) — Offensive. 150% ATK + bonus from cart weight. 5x5 AoE. SP: 12. Knockback 2 cells. Job Lv35.

**Change Cart** (rA: 154) — Active. Change cart appearance. SP: 40. Job Lv30.

**Crazy Uproar** (rA: 155) — Supportive. +4 STR, +30 ATK for 5 min. SP: 8. Job Lv15.

---

## Acolyte

### Heal
| Field | Value |
|-------|-------|
| ID | 400 (rA: 28) |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (self/ally/Undead enemy) |
| Range | 9 cells |
| Element | Holy |
| Cast Time | 0 |
| ACD | 1s |

**Heal Formula:** `floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)`

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 | 40 |
| Base Factor | 12 | 20 | 28 | 36 | 44 | 52 | 60 | 68 | 76 | 84 |

- Against Undead element: deals Holy damage = 50% of what would be healed (modified by element level).

### Divine Protection
| Field | Value |
|-------|-------|
| ID | 401 (rA: 22) |
| Type | Passive |
| Max Level | 10 |
| Description | +3 VIT DEF vs Undead/Demon per level. |

### Demon Bane
| Field | Value |
|-------|-------|
| ID | rA: 23 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK vs Undead/Demon per level (ignores DEF). |

### Blessing
| Field | Value |
|-------|-------|
| ID | 402 (rA: 34) |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (self/ally/enemy) |
| Range | 9 cells |
| Prerequisites | Divine Protection Lv5 |
| Cast Time | 0 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 | 64 |
| STR/DEX/INT+ | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Duration(s) | 60 | 80 | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |

- On Undead/Demon: reduces DEX and INT by 50% instead.
- Cures Curse and Stone (stage 2) status.

### Increase AGI
| Field | Value |
|-------|-------|
| ID | 403 (rA: 29) |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (self/ally) |
| Range | 9 cells |
| Prerequisites | Heal Lv3 |
| Cast Time | 1s |
| ACD | 1s |
| HP Cost | 15 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 18 | 21 | 24 | 27 | 30 | 33 | 36 | 39 | 42 | 45 |
| AGI+ | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Duration(s) | 60 | 80 | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 |

- Cancels Decrease AGI. Increases movement speed.

### Decrease AGI
| Field | Value |
|-------|-------|
| ID | 404 (rA: 30) |
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy |
| Cast Time | 1s |
| ACD | 1s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31 | 33 |
| AGI- | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Success% | 42 | 44 | 46 | 48 | 50 | 52 | 54 | 56 | 58 | 60 |

### Cure
| Field | Value |
|-------|-------|
| ID | 405 (rA: 35) |
| Type | Supportive |
| Max Level | 1 |
| SP Cost | 15 |
| Prerequisites | Heal Lv2 |
| Description | Removes Silence, Blind, Chaos. |

### Angelus
| Field | Value |
|-------|-------|
| ID | 406 (rA: 33) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self (party-wide) |
| Prerequisites | Divine Protection Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 23 | 27 | 31 | 35 | 39 | 43 | 47 | 51 | 55 | 59 |
| VIT DEF+% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| Duration(s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

### Signum Crucis
| Field | Value |
|-------|-------|
| ID | 407 (rA: 32) |
| Type | Active |
| Max Level | 10 |
| Target | AoE (screen-wide) |
| SP Cost | 35 |
| Description | Reduces DEF of Undead/Demon on screen. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| DEF-% | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| Success% | 27 | 31 | 35 | 39 | 43 | 47 | 51 | 55 | 59 | 63 |

### Ruwach
| Field | Value |
|-------|-------|
| ID | 408 (rA: 24) |
| Type | Offensive |
| Max Level | 1 |
| Target | Self (5x5 AoE) |
| Element | Holy |
| SP Cost | 10 |
| Duration | 10s |
| Damage | 145% MATK to revealed hidden enemies. |

### Teleport
| Field | Value |
|-------|-------|
| ID | 409 (rA: 26) |
| Type | Supportive |
| Max Level | 2 |
| Target | Self |
| SP Cost | 10 |
| Lv1 | Random location on current map |
| Lv2 | Save point |

### Warp Portal
| Field | Value |
|-------|-------|
| ID | 410 (rA: 27) |
| Type | Supportive |
| Max Level | 4 |
| Target | Ground |
| Range | 9 cells |
| SP Cost | 35 |
| Cast Time | 1s |
| Catalyst | 1 Blue Gemstone |
| Duration | ~10s (portal stays open) |

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| Memo Points | 1 | 2 | 3 | 3 + Save Point |

### Pneuma
| Field | Value |
|-------|-------|
| ID | 411 (rA: 25) |
| Type | Supportive |
| Max Level | 1 |
| Target | Ground (3x3) |
| Range | 9 cells |
| SP Cost | 10 |
| Prerequisites | Warp Portal Lv4 |
| Duration | 10s |
| Description | Blocks ALL ranged physical attacks in 3x3 area. Does not block magic. |

### Aqua Benedicta
| Field | Value |
|-------|-------|
| ID | 412 (rA: 31) |
| Type | Active |
| Max Level | 1 |
| SP Cost | 10 |
| Description | Create 1 Holy Water. Must stand in water. Consumes Empty Bottle. |

### Holy Light (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 156 |
| Type | Offensive |
| Max Level | 1 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Holy |
| SP Cost | 15 |
| Cast Time | 2s |
| Damage | 125% MATK |

---

## Knight

### Spear Mastery
| Field | Value |
|-------|-------|
| ID | 700 (rA: 55) |
| Type | Passive |
| Max Level | 10 |
| Description | +4 ATK/level with spears (unmounted), +5 ATK/level (mounted on Peco Peco). |

### Pierce
| Field | Value |
|-------|-------|
| ID | 701 (rA: 56) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Element | Neutral |
| SP Cost | 7 |
| Prerequisites | Spear Mastery Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 | 200 |

- Hits: Small = 3, Medium = 2, Large = 1 hit. Damage per hit = ATK%.
- Requires Spear weapon.

### Spear Stab
| Field | Value |
|-------|-------|
| ID | 702 (rA: 58) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| SP Cost | 9 |
| Prerequisites | Pierce Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

- Knockback 6 cells.

### Brandish Spear
| Field | Value |
|-------|-------|
| ID | 703 (rA: 57) |
| Type | Offensive |
| Max Level | 10 |
| Target | AoE (frontal cone) |
| SP Cost | 12 |
| Prerequisites | Pierce Lv5, Spear Stab Lv3, Riding Lv1 |
| Requirement | Must be mounted on Peco Peco |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

- AoE increases with level (wider frontal cone at higher levels).

### Spear Boomerang
| Field | Value |
|-------|-------|
| ID | 704 (rA: 59) |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 3-11 cells (increases per level) |
| SP Cost | 10 |
| Prerequisites | Pierce Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 150 | 200 | 250 | 300 | 350 |
| Range | 3 | 5 | 7 | 9 | 11 |

### Twohand Quicken
| Field | Value |
|-------|-------|
| ID | 705 (rA: 60) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Two-Handed Sword Mastery Lv1 |
| Requirement | Must wield 2H Sword |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| Duration(s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

- ASPD +30% while active.

### Counter Attack
| Field | Value |
|-------|-------|
| ID | 706 (rA: 61) |
| Type | Offensive |
| Max Level | 5 |
| Target | Self (counter-stance) |
| SP Cost | 3 |
| Prerequisites | Two-Handed Sword Mastery Lv1 |
| Description | Enter guard stance. Next melee attack received triggers a critical counter-attack. |

### Bowling Bash
| Field | Value |
|-------|-------|
| ID | 707 (rA: 62) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single (3x3 AoE) |
| Range | Melee |
| Element | Neutral |
| Prerequisites | Bash Lv10, Magnum Break Lv3, Two-Handed Sword Mastery Lv5, Twohand Quicken Lv10, Counter Attack Lv5 |
| Cast Time | 0.7s (uninterruptible) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |
| SP | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 |

- Can hit twice in AoE. Knockback 1 cell.

### Riding (Peco Peco Ride)
| Field | Value |
|-------|-------|
| ID | 708 (rA: 63) |
| Type | Passive |
| Max Level | 1 |
| Description | Enables mounting Peco Peco. Increases movement speed, reduces ASPD. |

### Cavalier Mastery
| Field | Value |
|-------|-------|
| ID | 709 (rA: 64) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Riding Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ASPD Recovery% | 60 | 70 | 80 | 90 | 100 |

- Restores ASPD lost from mounting.

### Charge Attack (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1001 |
| Type | Offensive |
| SP Cost | 40 |
| Damage | 100-600% based on distance. |
| Description | Ranged physical attack. Damage increases with distance. Job Lv40. |

---

## Crusader

### Faith
| Field | Value |
|-------|-------|
| ID | rA: 248 |
| Type | Passive |
| Max Level | 10 |
| Description | +200 MaxHP, +5% Holy resistance per level. |

### Auto Guard (Guard)
| Field | Value |
|-------|-------|
| ID | rA: 249 |
| Type | Supportive (Toggle) |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Shield required |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Block% | 5 | 8 | 11 | 14 | 17 | 20 | 23 | 26 | 28 | 30 |

### Shield Charge (Smite)
| Field | Value |
|-------|-------|
| ID | rA: 250 |
| Type | Offensive |
| Max Level | 5 |
| SP Cost | 10 |
| Prerequisites | Auto Guard Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |
| Stun% | 15 | 20 | 25 | 30 | 40 |

- Knockback 5 cells.

### Shield Boomerang
| Field | Value |
|-------|-------|
| ID | rA: 251 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 4-8 cells |
| SP Cost | 12 |
| Prerequisites | Shield Charge Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 130 | 160 | 190 | 220 | 250 |

- Damage based on ATK + shield properties.

### Reflect Shield (Shield Reflect)
| Field | Value |
|-------|-------|
| ID | rA: 252 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Shield Boomerang Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 35 | 40 | 45 | 50 | 55 | 60 | 65 | 70 | 75 | 80 |
| Reflect% | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 | 40 |
| Duration(s) | 300 | 300 | 300 | 300 | 300 | 300 | 300 | 300 | 300 | 300 |

### Holy Cross
| Field | Value |
|-------|-------|
| ID | rA: 253 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Element | Holy |
| Prerequisites | Faith Lv7 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 135 | 170 | 205 | 240 | 275 | 310 | 345 | 380 | 415 | 450 |
| SP | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
| Blind% | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

### Grand Cross
| Field | Value |
|-------|-------|
| ID | rA: 254 |
| Type | Offensive |
| Max Level | 10 |
| Target | AoE (5x5 self-centered) |
| Element | Holy |
| Prerequisites | Holy Cross Lv6, Faith Lv10 |
| Cast Time | 3s |
| ACD | 1.5s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 37 | 44 | 51 | 58 | 65 | 72 | 79 | 86 | 93 | 100 |
| (ATK+MATK)% | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |

- Costs 20% of MaxHP.
- Deals hybrid damage (ATK + MATK combined).

### Devotion (Sacrifice)
| Field | Value |
|-------|-------|
| ID | rA: 255 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Ally |
| SP Cost | 25 |
| Prerequisites | Reflect Shield Lv5, Grand Cross Lv4 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Targets | 1 | 2 | 3 | 4 | 5 |
| Duration(s) | 30 | 45 | 60 | 75 | 90 |

- Redirects damage from protected ally to the Crusader.

### Providence (Resistant Souls)
| Field | Value |
|-------|-------|
| ID | rA: 256 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 30 |
| Description | +5% per level resistance to Demon race and Holy element for party. |

### Defender (Defending Aura)
| Field | Value |
|-------|-------|
| ID | rA: 257 |
| Type | Supportive (Toggle) |
| Max Level | 5 |
| SP Cost | 30 |
| Prerequisites | Shield Boomerang Lv1 |
| Description | Reduces ranged damage by 20-80%. Reduces own move speed and ASPD. Affects nearby party. |

### Spear Quicken
| Field | Value |
|-------|-------|
| ID | rA: 258 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Spear Mastery Lv10 |
| Requirement | Must wield 2H Spear |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 |
| Duration(s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

### Shrink (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1002 |
| SP Cost | 15 |
| Description | When Auto Guard procs, 50% chance to knockback attacker 2 cells. |

---

## Wizard

### Fire Pillar
| Field | Value |
|-------|-------|
| ID | rA: 80 |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Element | Fire |
| SP Cost | 75 |
| AoE | 1x1 cell |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Cast(s) | 3.0 | 2.7 | 2.4 | 2.1 | 1.8 | 1.5 | 1.2 | 0.9 | 0.6 | 0.3 |
| Hits | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

- Each hit: 50% MATK + MATK/5 damage.
- Acts as a ground trap that triggers when stepped on.

### Sightrasher
| Field | Value |
|-------|-------|
| ID | rA: 81 |
| Type | Offensive |
| Max Level | 10 |
| Target | Self (AoE) |
| Element | Fire |
| Cast Time | 0.5s |
| Prerequisites | Sight Lv1 |
| Requirement | Sight must be active |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |
| SP | 35 | 37 | 39 | 41 | 43 | 45 | 47 | 49 | 51 | 53 |

- Knockback 5 cells.

### Jupitel Thunder
| Field | Value |
|-------|-------|
| ID | rA: 84 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Wind |
| Prerequisites | Napalm Beat Lv1, Lightning Bolt Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 23 | 26 | 29 | 32 | 35 | 38 | 41 | 44 | 47 |
| Cast(s) | 2.5 | 3.0 | 3.5 | 4.0 | 4.5 | 5.0 | 5.5 | 6.0 | 6.5 | 7.0 |
| Hits | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| Knockback | 2 | 3 | 3 | 4 | 4 | 5 | 5 | 6 | 6 | 7 |

- Each hit deals 1x MATK.

### Lord of Vermilion
| Field | Value |
|-------|-------|
| ID | rA: 85 |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Range | 9 cells |
| Element | Wind |
| AoE | 9x9 (effective 11x11) |
| Prerequisites | Jupitel Thunder Lv5 |
| ACD | 5s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 60 | 64 | 68 | 72 | 76 | 80 | 84 | 88 | 92 | 96 |
| Cast(s) | 15 | 14.5 | 14 | 13.5 | 13 | 12.5 | 12 | 11.5 | 11 | 10.5 |
| MATK% per wave | 100 | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 |
| Blind% | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

- 4 actual damage waves displayed as 10 visual hits each.

### Water Ball
| Field | Value |
|-------|-------|
| ID | rA: 86 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Element | Water |
| Prerequisites | Cold Bolt Lv1, Lightning Bolt Lv1 |
| Requirement | Must have water cells nearby |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 15 | 20 | 20 | 25 | 25 |
| Cast(s) | 1 | 2 | 3 | 4 | 5 |
| MATK% per hit | 130 | 160 | 190 | 220 | 250 |
| Max Hits | 1 | 4 | 9 | 9 | 25 |

### Ice Wall
| Field | Value |
|-------|-------|
| ID | rA: 87 |
| Type | Active |
| Max Level | 10 |
| Target | Ground |
| Element | Water |
| SP Cost | 20 |
| Prerequisites | Stone Curse Lv1, Frost Diver Lv1 |
| Description | Creates 5-cell ice wall. Each cell has 200 + 200*Level HP. Duration 5 + 5*Level seconds. |

### Frost Nova
| Field | Value |
|-------|-------|
| ID | rA: 88 |
| Type | Offensive |
| Max Level | 10 |
| Target | AoE (5x5 self-centered) |
| Element | Water |
| Prerequisites | Frost Diver Lv1, Ice Wall Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| MATK% | 73 | 80 | 87 | 94 | 101 | 108 | 115 | 122 | 129 | 136 |
| SP | 27 | 29 | 31 | 33 | 35 | 37 | 39 | 41 | 43 | 45 |
| Cast(s) | 6 | 5.6 | 5.2 | 4.8 | 4.4 | 4 | 4 | 4 | 4 | 4 |

- Freeze chance (same as Frost Diver third-hit mechanic).

### Storm Gust
| Field | Value |
|-------|-------|
| ID | rA: 89 |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Range | 9 cells |
| Element | Water |
| AoE | 9x9 (effective 11x11) |
| SP Cost | 78 |
| Prerequisites | Frost Diver Lv1, Jupitel Thunder Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Cast(s) | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
| MATK% per hit | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |

- 10 hits total.
- Every 3rd hit has a 150% chance to Freeze target (boss/undead immune).

### Meteor Storm
| Field | Value |
|-------|-------|
| ID | rA: 83 |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground |
| Range | 9 cells |
| Element | Fire |
| AoE | 7x7 (potential 13x13) |
| Prerequisites | Sightrasher Lv2, Thunderstorm Lv1 |
| ACD | 2 + 0.5*Level seconds |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 24 | 30 | 34 | 40 | 44 | 50 | 54 | 60 | 64 |
| Meteors | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 | 6 | 6 |
| Hits/Meteor | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 |

- Each hit deals 125% MATK. Chance to Stun.

### Earth Spike
| Field | Value |
|-------|-------|
| ID | rA: 90 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Element | Earth |
| Prerequisites | None (Wizard/Sage innate) |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 |
| Cast(s) | 0.7 | 1.4 | 2.1 | 2.8 | 3.5 |
| Spikes | 1 | 2 | 3 | 4 | 5 |

- Each spike deals 1x MATK.

### Heaven's Drive
| Field | Value |
|-------|-------|
| ID | rA: 91 |
| Type | Offensive |
| Max Level | 5 |
| Target | Ground (5x5) |
| Element | Earth |
| Prerequisites | Earth Spike Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 28 | 32 | 36 | 40 | 44 |
| Cast(s) | 1 | 2 | 3 | 4 | 5 |
| MATK% | 125 | 250 | 375 | 500 | 625 |

### Quagmire
| Field | Value |
|-------|-------|
| ID | rA: 92 |
| Type | Active |
| Max Level | 5 |
| Target | Ground (5x5) |
| Element | Earth |
| Prerequisites | Heaven's Drive Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 5 | 10 | 15 | 20 | 25 |
| AGI/DEX Reduction | 5 | 10 | 15 | 20 | 25 |
| Duration(s) | 5 | 10 | 15 | 20 | 25 |

### Sense (Monster Property)
| Field | Value |
|-------|-------|
| ID | rA: 93 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 10 |
| Description | Reveal monster element, race, size, HP. |

### Sight Blaster (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1006 |
| Type | Offensive |
| SP Cost | 40 |
| Element | Fire |
| Description | Stores an attack. Next enemy within 2 cells takes 100% MATK damage + knockback. Job Lv30. |

---

## Sage

### Advanced Book (Study)
| Field | Value |
|-------|-------|
| ID | rA: 274 |
| Type | Passive |
| Max Level | 5 |
| Description | +3 ATK, +0.5% ASPD per level with Book weapons. |

### Cast Cancel
| Field | Value |
|-------|-------|
| ID | rA: 275 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 2 |
| Description | Cancel own casting. SP refund: 65% at Lv1, +5% per level (90% at Lv5). |

### Magic Rod
| Field | Value |
|-------|-------|
| ID | rA: 276 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 2 |
| Description | Absorb the next single-target magic spell cast on you and convert it to SP. |

### Spell Breaker
| Field | Value |
|-------|-------|
| ID | rA: 277 |
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| SP Cost | 10 |
| Description | Interrupt enemy's casting and absorb the SP cost. |

### Free Cast
| Field | Value |
|-------|-------|
| ID | rA: 278 |
| Type | Passive |
| Max Level | 10 |
| Description | Move and attack while casting spells. Move speed during cast: Lv1=30%, Lv10=100%. |

### Auto Spell (Hindsight)
| Field | Value |
|-------|-------|
| ID | rA: 279 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| SP Cost | 35 |
| Duration | 300s |
| Description | Auto-cast bolt spells on melee hit. Available bolt/level increases with skill level. |

### Endow Blaze / Frost Weapon / Lightning Loader / Seismic Weapon
| ID | rA: 280-283 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 40 |
| Target | Single Ally |
| Duration(s) | 20-60 (per level) |
| Catalyst | Elemental Stone (Red Blood / Crystal Blue / Wind of Verdure / Green Live) |
| Description | Enchant target's weapon with Fire/Water/Wind/Earth element. |

### Volcano / Deluge / Violent Gale
| ID | rA: 285-287 |
| Type | Active |
| Max Level | 5 |
| Target | Ground (7x7) |
| SP Cost | 40-48 |
| Catalyst | 1 Yellow Gemstone |
| Description | Create elemental ground effect boosting corresponding element damage 10-50%. |

### Land Protector (Magnetic Earth)
| Field | Value |
|-------|-------|
| ID | rA: 288 |
| Type | Active |
| Max Level | 5 |
| Target | Ground |
| SP Cost | 50-66 |
| Catalyst | 1 Blue Gemstone + 1 Yellow Gemstone |
| Description | Remove and prevent all ground-targeted spells in area. |

### Dispell
| Field | Value |
|-------|-------|
| ID | rA: 289 |
| Type | Active |
| Max Level | 5 |
| Target | Single |
| SP Cost | 1 |
| Catalyst | 1 Yellow Gemstone |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Success% | 60 | 70 | 80 | 90 | 100 |

### Dragonology
| Field | Value |
|-------|-------|
| ID | rA: 284 |
| Type | Passive |
| Max Level | 5 |
| Description | +3 INT, +20% ATK vs Dragon race at Lv5. +5% dragon resist/level. |

### Hocus-Pocus (Abracadabra)
| Field | Value |
|-------|-------|
| ID | rA: 290 |
| Type | Active |
| Max Level | 10 |
| SP Cost | 50 |
| Catalyst | 2 Yellow Gemstones |
| Description | Cast a random non-transcendent skill at your level. Chaotic results. |

### Quest Skills

**Create Elemental Converter** (rA: 1007) — SP: 30. Create elemental converter items.

**Elemental Change** (rA: 1008, 1017-1019) — SP: 30. Change target monster element. Job Lv40.

---

## Hunter

### Beast Bane
| Field | Value |
|-------|-------|
| ID | rA: 126 |
| Type | Passive |
| Max Level | 10 |
| Description | +4 ATK per level vs Brute and Insect race. |

### Falconry Mastery
| Field | Value |
|-------|-------|
| ID | rA: 127 |
| Type | Passive |
| Max Level | 1 |
| Description | Enables renting and commanding a Falcon. |

### Steel Crow
| Field | Value |
|-------|-------|
| ID | rA: 128 |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Falconry Mastery Lv1 |
| Description | +6 Falcon damage per level. |

### Blitz Beat
| Field | Value |
|-------|-------|
| ID | rA: 129 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single (3x3 splash) |
| Prerequisites | Falconry Mastery Lv1, Steel Crow Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 10 | 13 | 16 | 19 | 22 |
| Hits | 1 | 2 | 3 | 4 | 5 |

- **Damage formula:** `((DEX/10) + (INT/2) + Steel Crow Bonus + 40) * Hits`
- Can also proc passively (auto-blitz) when attacking.

### Detect
| Field | Value |
|-------|-------|
| ID | rA: 130 |
| Type | Active |
| Max Level | 4 |
| SP Cost | 8 |
| Description | Falcon reveals hidden enemies. Range: 2-8 cells. |

### Trap Skills

All trap skills require 1 Trap item (purchasable from NPC).

| Skill | ID | Max Lv | SP | Type | Effect |
|-------|-----|--------|-----|------|--------|
| Skid Trap | 115 | 5 | 10 | Active | Slides enemy 6-10 cells |
| Land Mine | 116 | 5 | 10 | Offensive | Earth damage: (DEX+75)*(1+INT/100)*Lv |
| Ankle Snare | 117 | 5 | 12 | Active | Immobilizes target. Duration: 5*Lv/(AGI*0.1) |
| Shockwave Trap | 118 | 5 | 45 | Active | Drains (5+15*Lv)% SP |
| Sandman | 119 | 5 | 12 | Active | Sleep (40+10*Lv)% chance. 5x5 AoE |
| Flasher | 120 | 5 | 12 | Active | Blind status. 5x5 AoE |
| Freezing Trap | 121 | 5 | 10 | Offensive | Water damage (25+25*Lv)% ATK + Freeze chance |
| Blast Mine | 122 | 5 | 10 | Offensive | Wind damage: (50+DEX/2)*(1+INT/100)*Lv. 3x3 AoE |
| Claymore Trap | 123 | 5 | 15 | Offensive | Fire damage: (75+DEX/2)*(1+INT/100)*Lv. 5x5 AoE |
| Remove Trap | 124 | 1 | 5 | Active | Recover own trap |
| Talkie Box | 125 | 1 | 1 | Active | Display message on trigger |
| Spring Trap | 131 | 5 | 10 | Active | Falcon removes traps at range 4-8 cells |

### Phantasmic Arrow (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1009 |
| SP Cost | 10 |
| Damage | 150% ATK (no arrow consumed). Knockback 3 cells. |

---

## Bard

### Music Lessons
| Field | Value |
|-------|-------|
| ID | rA: 315 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK/level with Instruments. Improves performance skill effects. +1 MaxSP per 5 levels. |

### Musical Strike (Melody Strike)
| Field | Value |
|-------|-------|
| ID | rA: 316 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 9 cells |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 150 | 190 | 230 | 270 | 310 |
| SP | 1 | 3 | 5 | 7 | 9 |

### Frost Joker (Unbarring Octave)
| Field | Value |
|-------|-------|
| ID | rA: 318 |
| Type | Active |
| Max Level | 5 |
| Target | Screen-wide |
| Description | Chance to Freeze all enemies (and allies!) on screen. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 |
| Freeze% | 20 | 25 | 30 | 35 | 40 |

### Performance Skills (Solo — Bard)

| Skill | ID | Max Lv | SP Range | AoE | Effect |
|-------|-----|--------|----------|-----|--------|
| A Whistle (Perfect Tablature) | 319 | 10 | 22-40 | 7x7 | +FLEE, +Perfect Dodge |
| Assassin Cross of Sunset (Magic Strings) | 320 | 10 | 40-85 | 7x7 | Reduces After-Attack Delay |
| A Poem of Bragi | 321 | 10 | 40-85 | 7x7 | Reduces Cast Time and Skill Delay |
| The Apple of Idun (Song of Lutie) | 322 | 10 | 40-85 | 7x7 | +MaxHP, +HP Recovery |

### Amp (Adaptation to Circumstances)
| Field | Value |
|-------|-------|
| ID | rA: 304 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 1 |
| Description | Cancel current performance. |

### Encore
| Field | Value |
|-------|-------|
| ID | rA: 305 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 1 (half of last performance cost) |
| Description | Recast last performance at half SP. |

### Pang Voice (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1010 |
| SP Cost | 20 |
| Description | Inflict Confusion status on target. |

---

## Dancer

### Dancing Lessons
| Field | Value |
|-------|-------|
| ID | rA: 323 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK/level with Whips. Improves performance skills. |

### Throw Arrow (Slinging Arrow)
| Field | Value |
|-------|-------|
| ID | rA: 324 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| SP Cost | 12 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 150 | 190 | 230 | 270 | 310 |

### Scream (Dazzler)
| Field | Value |
|-------|-------|
| ID | rA: 326 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 20 |
| Description | Chance to Stun all enemies on screen. |

### Performance Skills (Solo — Dancer)

| Skill | ID | Max Lv | SP Range | AoE | Effect |
|-------|-----|--------|----------|-----|--------|
| Humming (Focus Ballet) | 327 | 10 | 22-60 | 7x7 | +HIT |
| Please Don't Forget Me (Slow Grace) | 328 | 10 | 28-65 | 7x7 | -Enemy ASPD and Movement |
| Fortune's Kiss (Lady Luck) | 329 | 10 | 40-85 | 7x7 | +CRIT |
| Service For You (Gypsy's Kiss) | 330 | 10 | 40-87 | 7x7 | +MaxSP, -SP Consumption |

### Hip Shaker (Ugly Dance)
| Field | Value |
|-------|-------|
| ID | rA: 325 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 35 |
| Description | Drains SP from all enemies in 7x7. PvP/WoE focused. |

### Charming Wink (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1011 |
| SP Cost | 40 |
| Description | Charm a target for 10 seconds. |

### Ensemble Skills (Bard + Dancer together)

| Skill | ID | Max Lv | SP | AoE | Effect |
|-------|-----|--------|-----|-----|--------|
| Lullaby | 306 | 1 | 20-40 | 9x9 | Sleep enemies |
| Mr. Kim A Rich Man (Mental Sensing) | 307 | 5 | 62-86 | 9x9 | +EXP 20-60% |
| Eternal Chaos (Down Tempo) | 308 | 1 | 30-120 | 9x9 | Enemy DEF to 0 |
| A Drum on the Battlefield (Battle Theme) | 309 | 5 | 38-50 | 9x9 | +ATK, +DEF |
| The Ring of Nibelungen (Harmonic Lick) | 310 | 5 | 48-64 | 9x9 | Lv4 weapon bonus |
| Loki's Veil (Classical Pluck) | 311 | 1 | 15-180 | 9x9 | Disable all skills |
| Into the Abyss (Power Cord) | 312 | 1 | 10-70 | 9x9 | No gemstone costs |
| Invulnerable Siegfried (Acoustic Rhythm) | 313 | 5 | 20-56 | 9x9 | Element/status resist |

---

## Assassin

### Katar Mastery
| Field | Value |
|-------|-------|
| ID | rA: 134 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK per level with Katars (ignores DEF). |

### Right-Hand Mastery
| Field | Value |
|-------|-------|
| ID | rA: 132 |
| Type | Passive |
| Max Level | 5 |
| Description | Recovers dual-wield right hand damage penalty: 60/70/80/90/100% at Lv1-5. |

### Left-Hand Mastery
| Field | Value |
|-------|-------|
| ID | rA: 133 |
| Type | Passive |
| Max Level | 5 |
| Description | Recovers dual-wield left hand damage penalty: 40/50/60/70/80% at Lv1-5. |

### Cloaking
| Field | Value |
|-------|-------|
| ID | rA: 135 |
| Type | Active (Toggle) |
| Max Level | 10 |
| Target | Self |
| SP Cost | 15 |
| Description | Become invisible. Must be near a wall at Lv1-2. Movement speed increases per level. |

### Sonic Blow
| Field | Value |
|-------|-------|
| ID | rA: 136 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Element | Weapon |
| Prerequisites | Katar Mastery Lv4 |
| Cast Time | 0 |
| ACD | 2s |
| Requirement | Katar weapon |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 440 | 480 | 520 | 560 | 600 | 640 | 680 | 720 | 760 | 800 |
| SP | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 | 32 | 34 |
| Stun% | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |

- 8 visual hits (damage calculated as one).

### Grimtooth
| Field | Value |
|-------|-------|
| ID | rA: 137 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single (3x3 splash) |
| Range | 3-7 cells |
| SP Cost | 3 |
| Prerequisites | Katar Mastery Lv5, Cloaking Lv2 |
| Requirement | Must be in Hiding |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |

### Enchant Poison
| Field | Value |
|-------|-------|
| ID | rA: 138 |
| Type | Supportive |
| Max Level | 10 |
| SP Cost | 20 |
| Duration | 60-150s (30 + 30*Lv/5 approx.) |
| Description | Weapon becomes Poison element. Chance to Poison on hit: 2.5 + 0.5*Lv %. |

### Poison React
| Field | Value |
|-------|-------|
| ID | rA: 139 |
| Type | Offensive |
| Max Level | 10 |
| SP Cost | 25-45 |
| Description | Counter-attack on receiving Poison element damage: (130+30*Lv)% ATK. Auto-Envenom response for non-poison attacks. |

### Venom Dust
| Field | Value |
|-------|-------|
| ID | rA: 140 |
| Type | Active |
| Max Level | 10 |
| Target | Ground (2x2) |
| SP Cost | 20 |
| Catalyst | 1 Red Gemstone |
| Duration | 5-50s (5*Lv) |
| Description | Create poison cloud. Poisons enemies standing in it. |

### Venom Splasher
| Field | Value |
|-------|-------|
| ID | rA: 141 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| SP Cost | 12-30 |
| Catalyst | 1 Red Gemstone |
| Prerequisites | Venom Dust Lv5 |
| Description | After delay, target explodes for (500+50*Lv)% ATK in 5x5. Target must be at <3/4 HP. |

### Quest Skills

**Sonic Acceleration** (rA: 1003) — Passive. +50% damage on Sonic Blow. Job Lv30.

**Venom Knife** (rA: 1004) — Offensive. SP: 35. Ranged poison attack. Job Lv35.

---

## Rogue

### Sword Mastery
Same as Swordsman (shared skill). +4 ATK/level with Daggers and 1H Swords.

### Vulture's Eye
Same as Archer (shared skill).

### Double Strafe
Same as Archer (shared skill). Requires Vulture's Eye Lv10 for Rogues.

### Snatcher (Gank)
| Field | Value |
|-------|-------|
| ID | rA: 210 |
| Type | Passive |
| Max Level | 10 |
| Description | 7-20% chance to auto-Steal when attacking. |

### Steal Coin (Mug)
| Field | Value |
|-------|-------|
| ID | rA: 211 |
| Type | Active |
| Max Level | 10 |
| SP Cost | 15 |
| Description | Steal Zeny from monster. 1-10% success/level. |

### Tunnel Drive (Stalk)
| Field | Value |
|-------|-------|
| ID | rA: 213 |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Hiding Lv1 |
| Description | Move while in Hiding. Speed: 26-50% per level. |

### Back Stab
| Field | Value |
|-------|-------|
| ID | rA: 212 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| SP Cost | 16 |
| Description | Attack from behind. Never misses. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 340 | 380 | 420 | 460 | 500 | 540 | 580 | 620 | 660 | 700 |

### Raid (Sightless Mind)
| Field | Value |
|-------|-------|
| ID | rA: 214 |
| Type | Offensive |
| Max Level | 5 |
| Target | AoE (3x3 self-centered) |
| SP Cost | 15-20 |
| Prerequisites | Back Stab Lv2, Tunnel Drive Lv2 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 |

### Strip Skills (Divest)

| Skill | ID | Max Lv | SP | Target | Success% |
|-------|-----|--------|-----|--------|---------|
| Strip Weapon | 215 | 5 | 17-25 | Single | 7-15% |
| Strip Shield | 216 | 5 | 12-20 | Single | 7-15% |
| Strip Armor | 217 | 5 | 17-25 | Single | 7-15% |
| Strip Helm | 218 | 5 | 12-20 | Single | 7-15% |

### Intimidate (Plagiarism passive)
| Field | Value |
|-------|-------|
| ID | rA: 219 |
| Type | Offensive |
| Max Level | 10 |
| SP Cost | 13-25 |
| Description | Attack + teleport self and target to random spot. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 130 | 150 | 170 | 190 | 210 | 230 | 250 | 270 | 290 | 310 |

### Plagiarism (Intimidate Passive)
| Field | Value |
|-------|-------|
| ID | rA: 225 |
| Type | Passive |
| Max Level | 10 |
| Description | Copy the last skill that damaged you. Can use it as your own. |

### Compulsion Discount (Haggle)
| Field | Value |
|-------|-------|
| ID | rA: 224 |
| Type | Passive |
| Max Level | 5 |
| Description | 9-25% NPC buy discount. |

### Gangster's Paradise
| Field | Value |
|-------|-------|
| ID | rA: 223 |
| Type | Passive |
| Max Level | 1 |
| Description | When 2+ Rogues sit together, nearby non-boss monsters will not attack. |

### Close Confine (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1005 |
| SP Cost | 25 |
| Duration | 10s |
| Description | Lock both caster and target in place. |

---

## Blacksmith

### Weaponry Research
| Field | Value |
|-------|-------|
| ID | rA: 107 |
| Type | Passive |
| Max Level | 10 |
| Description | +2 HIT, +2 ATK per level. |

### Skin Tempering
| Field | Value |
|-------|-------|
| ID | rA: 109 |
| Type | Passive |
| Max Level | 5 |
| Description | +4% Fire resistance per level (20% at Lv5). +1% Neutral resist/level. |

### Hilt Binding
| Field | Value |
|-------|-------|
| ID | rA: 105 |
| Type | Passive |
| Max Level | 1 |
| Description | +1 STR, +4 ATK. Extends duration of Adrenaline Rush, Power-Thrust, Weapon Perfection by 10%. |

### Hammer Fall
| Field | Value |
|-------|-------|
| ID | rA: 110 |
| Type | Active |
| Max Level | 5 |
| Target | Ground (5x5) |
| SP Cost | 10 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Stun% | 30 | 40 | 50 | 60 | 70 |

### Adrenaline Rush
| Field | Value |
|-------|-------|
| ID | rA: 111 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| Prerequisites | Hammer Fall Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 20 | 23 | 26 | 29 | 32 |
| Duration(s) | 30 | 60 | 90 | 120 | 150 |

- +30% ASPD with Axe/Mace weapons. Party members get half effect.

### Weapon Perfection
| Field | Value |
|-------|-------|
| ID | rA: 112 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 18-10 (decreases) |
| Description | Remove weapon size penalty (always 100% damage regardless of target size). |

### Power-Thrust (Power Maximize)
| Field | Value |
|-------|-------|
| ID | rA: 113 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 18-10 |
| Description | +5-25% ATK for party. Chance to break own weapon on attack. |

### Maximize Power
| Field | Value |
|-------|-------|
| ID | rA: 114 |
| Type | Supportive (Toggle) |
| Max Level | 5 |
| SP Cost | 10 |
| Description | Always deal maximum weapon damage. Drains 1 SP per 1-5 seconds. |

### Forging Skills

| Skill | ID | Max Lv | Description |
|-------|-----|--------|-------------|
| Iron Tempering | 94 | 5 | Refine Iron Ore into Iron |
| Steel Tempering | 95 | 5 | Refine Iron + Coal into Steel |
| Enchantedstone Craft | 96 | 5 | Create elemental stones |
| Oridecon Research | 97 | 5 | Boost Lv3 weapon forge chance |
| Smith Dagger | 98 | 3 | Forge daggers |
| Smith Sword | 99 | 3 | Forge 1H swords |
| Smith Two-Handed Sword | 100 | 3 | Forge 2H swords |
| Smith Axe | 101 | 3 | Forge axes |
| Smith Mace | 102 | 3 | Forge maces |
| Smith Knuckle | 103 | 3 | Forge knuckles |
| Smith Spear | 104 | 3 | Forge spears |

### Repair Weapon
| Field | Value |
|-------|-------|
| ID | rA: 108 |
| SP Cost | 30 |
| Description | Repair broken weapon or armor. |

### Ore Discovery
| Field | Value |
|-------|-------|
| ID | rA: 106 |
| Type | Passive |
| Max Level | 1 |
| Description | Chance to find ores when killing monsters. |

### Quest Skills

**Greed** (rA: 1013) — Active. SP: 10. Pick up all items in 5x5 area. Job Lv30.

**Dubious Salesmanship** (rA: 1012) — Passive. Reduces Mammonite Zeny cost by 10%. Job Lv25.

---

## Alchemist

### Axe Mastery
| Field | Value |
|-------|-------|
| ID | rA: 226 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK/level with Axes (ignores DEF). |

### Potion Research (Learning Potion)
| Field | Value |
|-------|-------|
| ID | rA: 227 |
| Type | Passive |
| Max Level | 10 |
| Description | Increases potion creation success rate and healing effectiveness. |

### Prepare Potion (Pharmacy)
| Field | Value |
|-------|-------|
| ID | rA: 228 |
| Type | Active |
| Max Level | 10 |
| SP Cost | 5 |
| Description | Craft potions using Medicine Bowl + ingredients. Success based on stats + Potion Research. |

### Acid Terror
| Field | Value |
|-------|-------|
| ID | rA: 230 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 9 cells |
| SP Cost | 15 |
| Catalyst | 1 Acid Bottle |
| Description | Ignores armor DEF. Always hits. Chance to break armor, chance to cause Bleeding. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 160 | 200 | 240 | 300 |
| Armor Break% | 3 | 7 | 10 | 12 | 13 |

### Demonstration (Bomb)
| Field | Value |
|-------|-------|
| ID | rA: 229 |
| Type | Offensive |
| Max Level | 5 |
| Target | Ground (3x3) |
| Range | 9 cells |
| SP Cost | 10 |
| Element | Fire |
| Catalyst | 1 Bottle Grenade |
| Description | Fire ground DoT. Chance to break enemy weapons. |

### Potion Pitcher (Aid Potion)
| Field | Value |
|-------|-------|
| ID | rA: 231 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Ally |
| SP Cost | 1 |
| Description | Throw a potion to heal ally. Effectiveness scales with Potion Research. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Potion Used | Red | Orange | Yellow | White | Blue (SP) |

### Bio Cannibalize (Summon Flora)
| Field | Value |
|-------|-------|
| ID | rA: 232 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 20 |
| Catalyst | 1 Plant Bottle |
| Description | Summon 1-5 plant monsters depending on level. |

### Sphere Mine (Summon Marine Sphere)
| Field | Value |
|-------|-------|
| ID | rA: 233 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 10 |
| Catalyst | 1 Marine Sphere Bottle |
| Description | Summon a Marine Sphere that explodes on contact. |

### Chemical Protection (Weapon/Shield/Armor/Helm)

| Skill | ID | Max Lv | SP | Duration |
|-------|-----|--------|-----|----------|
| CP Weapon | 234 | 5 | 30 | 120-600s |
| CP Shield | 235 | 5 | 25 | 120-600s |
| CP Armor | 236 | 5 | 25 | 120-600s |
| CP Helm | 237 | 5 | 20-25 | 120-600s |

- Each requires 1 Glistening Coat. Prevents equipment breaking/stripping.

### Homunculus Skills

| Skill | ID | Description |
|-------|-----|-------------|
| Bioethics | 238 | Passive. Unlocks homunculus tree. Quest. |
| Call Homunculus | 243 | SP: 10. Summon stored homunculus. |
| Vaporize (Rest) | 244 | SP: 50. Store homunculus. |
| Resurrect Homunculus | 247 | SP: 50-74. Revive dead homunculus. |

---

## Priest

### Mace Mastery
| Field | Value |
|-------|-------|
| ID | rA: 65 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK/level with Maces (ignores DEF). |

### Impositio Manus
| Field | Value |
|-------|-------|
| ID | rA: 66 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single (self/ally) |
| Duration | 60s |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 13 | 16 | 19 | 22 | 25 |
| +ATK | 5 | 10 | 15 | 20 | 25 |

### Suffragium
| Field | Value |
|-------|-------|
| ID | rA: 67 |
| Type | Supportive |
| Max Level | 3 |
| Target | Single Ally (NOT self) |
| SP Cost | 8 |
| Description | Reduces next spell cast time by 15/30/45%. |

### Aspersio
| Field | Value |
|-------|-------|
| ID | rA: 68 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single (self/ally) |
| Catalyst | 1 Holy Water |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 14 | 18 | 22 | 26 | 30 |
| Duration(s) | 60 | 90 | 120 | 150 | 180 |

- Enchants weapon with Holy element.

### B.S. Sacramenti
| Field | Value |
|-------|-------|
| ID | rA: 69 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 20 |
| Description | Enchant target's armor with Holy element. Requires 2 Acolyte-class chars. |

### Kyrie Eleison
| Field | Value |
|-------|-------|
| ID | rA: 73 |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (self/ally) |
| Cast Time | 2s |
| Duration | 120s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 20 | 20 | 25 | 25 | 25 | 30 | 30 | 30 | 35 |
| MaxHP% Shield | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Max Hits | 5 | 6 | 6 | 7 | 7 | 8 | 8 | 9 | 9 | 10 |

### Magnificat
| Field | Value |
|-------|-------|
| ID | rA: 74 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| SP Cost | 40 |
| Cast Time | 4s |
| Description | Doubles natural HP and SP regeneration rate. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 30 | 45 | 60 | 75 | 90 |

### Gloria
| Field | Value |
|-------|-------|
| ID | rA: 75 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| SP Cost | 20 |
| Description | +30 LUK to all party members. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 10 | 15 | 20 | 25 | 30 |

### Lex Divina
| Field | Value |
|-------|-------|
| ID | rA: 76 |
| Type | Active |
| Max Level | 10 |
| Target | Single |
| Range | 5 cells |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 20 | 20 | 20 | 20 | 18 | 16 | 14 | 12 | 10 |
| Silence(s) | 30 | 35 | 40 | 45 | 50 | 50 | 50 | 50 | 55 | 60 |

### Lex Aeterna
| Field | Value |
|-------|-------|
| ID | rA: 78 |
| Type | Supportive |
| Max Level | 1 |
| Target | Single Enemy |
| SP Cost | 10 |
| Prerequisites | Lex Divina Lv5 |
| Description | Next hit on target deals DOUBLE damage. Removed after 1 hit. |

### Turn Undead
| Field | Value |
|-------|-------|
| ID | rA: 77 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy (Undead only) |
| SP Cost | 20 |
| Cast Time | 1s |
| Element | Holy |
| Description | Chance to instantly kill Undead. If fails: (BaseLv + INT + SkillLv*10) Holy damage. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Instakill% base | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |

- Full formula: `(20 + Lv + INT + LUK) / 1000 * SkillLv`

### Magnus Exorcismus
| Field | Value |
|-------|-------|
| ID | rA: 79 |
| Type | Offensive |
| Max Level | 10 |
| Target | Ground (7x7) |
| Element | Holy |
| Cast Time | 15s |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Turn Undead Lv3, Lex Aeterna Lv1 |
| Description | Damages Demon race and Undead element monsters only. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 40 | 42 | 44 | 46 | 48 | 50 | 52 | 54 | 56 | 58 |
| Waves | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Duration(s) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |

- Each wave deals 1x MATK Holy damage at 3-second intervals.

### Sanctuary
| Field | Value |
|-------|-------|
| ID | rA: 70 |
| Type | Supportive |
| Max Level | 10 |
| Target | Ground (5x5) |
| Cast Time | 5s |
| Catalyst | 1 Blue Gemstone |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 15 | 18 | 21 | 24 | 27 | 30 | 33 | 36 | 39 | 42 |
| Heal/tick | 100 | 200 | 300 | 400 | 500 | 600 | 777 | 777 | 777 | 777 |
| Duration(s) | 4 | 7 | 10 | 13 | 16 | 19 | 22 | 25 | 28 | 31 |

- Also damages Undead element monsters.

### Resurrection
| Field | Value |
|-------|-------|
| ID | rA: 54 |
| Type | Supportive |
| Max Level | 4 |
| Target | Dead Player |
| Catalyst | 1 Blue Gemstone |

| Level | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|
| SP | 60 | 60 | 60 | 60 |
| Cast(s) | 6 | 4 | 2 | 0 |
| Revive HP% | 10 | 30 | 50 | 80 |

### Slow Poison
| Field | Value |
|-------|-------|
| ID | rA: 71 |
| Type | Supportive |
| Max Level | 4 |
| SP Cost | 6-12 |
| Duration | 10-40s |
| Description | Halts poison HP drain temporarily. |

### Status Recovery
| Field | Value |
|-------|-------|
| ID | rA: 72 |
| Type | Supportive |
| Max Level | 1 |
| SP Cost | 5 |
| Cast Time | 2s |
| Description | Cure Frozen, Stoned, and Stun. |

### Redemptio (Quest Skill)
| Field | Value |
|-------|-------|
| ID | rA: 1014 |
| SP Cost | 400 |
| Description | Sacrifice self to resurrect all dead party members nearby at 50% HP. |

---

## Monk

### Iron Fists (Iron Hand)
| Field | Value |
|-------|-------|
| ID | rA: 259 |
| Type | Passive |
| Max Level | 10 |
| Description | +3 ATK/level with Knuckle weapons / bare hands. |

### Spiritual Cadence (Spirit Recovery)
| Field | Value |
|-------|-------|
| ID | rA: 260 |
| Type | Passive |
| Max Level | 5 |
| Description | Regen HP/SP while sitting regardless of weight. Rate increases per level. |

### Summon Spirit Sphere
| Field | Value |
|-------|-------|
| ID | rA: 261 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 8 |
| Description | Summon 1 spirit sphere per cast. Max spheres = skill level (1-5). Each sphere: +3 ATK. |

### Absorb Spirit Sphere
| Field | Value |
|-------|-------|
| ID | rA: 262 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 5 |
| Description | Absorb all spheres to recover 7 SP per sphere. Can absorb enemy Monk's spheres. |

### Triple Attack (Raging Trifecta Blow)
| Field | Value |
|-------|-------|
| ID | rA: 263 |
| Type | Passive |
| Max Level | 10 |
| Description | Chance to triple-hit on normal attack. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Proc% | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 29 |
| 3-hit ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

### Dodge (Flee skill)
| Field | Value |
|-------|-------|
| ID | rA: 265 |
| Type | Passive |
| Max Level | 10 |
| Description | +1-15 FLEE. |

### Occult Impaction (Investigate)
| Field | Value |
|-------|-------|
| ID | rA: 266 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Cost | 1 Spirit Sphere |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 10 | 14 | 17 | 19 | 20 |
| ATK% | 350 | 500 | 650 | 800 | 950 |

- Damage increases with enemy's DEF (uses DEF in damage calc instead of subtracting).

### Raging Quadruple Blow (Chain Combo)
| Field | Value |
|-------|-------|
| ID | rA: 272 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Prerequisites | Triple Attack Lv5 |
| Trigger | Must follow a Triple Attack proc |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 5 | 6 | 7 | 8 | 9 |
| 4-hit ATK% | 200 | 250 | 300 | 350 | 400 |

### Raging Thrust (Combo Finish)
| Field | Value |
|-------|-------|
| ID | rA: 273 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Cost | 1 Spirit Sphere |
| Prerequisites | Chain Combo Lv3 |
| Trigger | Must follow Chain Combo |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 3 | 4 | 5 | 6 | 7 |
| ATK% | 300 | 360 | 540 | 720 | 900 |

### Body Relocation (Snap)
| Field | Value |
|-------|-------|
| ID | rA: 264 |
| Type | Active |
| Max Level | 1 |
| Target | Ground cell |
| SP Cost | 14 |
| Cost | 1 Spirit Sphere |
| Description | Instant teleport to target cell. Requires Fury state. |

### Mental Strength (Steel Body)
| Field | Value |
|-------|-------|
| ID | rA: 268 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 200 |
| Cost | 5 Spirit Spheres |
| Duration | 30-150s (30*Lv) |
| Description | DEF and MDEF set to 90. Cannot use skills, items, or chat. Movement greatly slowed. |

### Explosion Spirits (Fury)
| Field | Value |
|-------|-------|
| ID | rA: 270 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 15 |
| Cost | 5 Spirit Spheres |
| Duration | 180s |
| Description | +CRIT (10 + 2*Lv). Enables Guillotine Fist and Snap. |

### Blade Stop (Root)
| Field | Value |
|-------|-------|
| ID | rA: 269 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 10 |
| Description | Catch enemy's attack mid-swing. Both immobilized 0.5-1.3s per level. |

### Throw Spirit Sphere (Finger Offensive)
| Field | Value |
|-------|-------|
| ID | rA: 267 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 9 cells |
| Cost | 1-5 Spirit Spheres (= skill level) |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 10 | 14 | 17 | 20 | 28 |

- Each sphere deals ATK-based damage.

### Guillotine Fist (Asura Strike)
| Field | Value |
|-------|-------|
| ID | rA: 271 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (2 cells) |
| Cost | 5 Spirit Spheres + ALL remaining SP |
| Prerequisites | Throw Spirit Sphere Lv3, Fury Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Cast(s) | 4.0 | 3.5 | 3.0 | 2.5 | 2.0 |
| Bonus | +400 | +550 | +700 | +850 | +1000 |

- **Damage formula:** `ATK * (8 + SP/10) + Bonus`
- Cannot naturally regen SP for 5 minutes after use.
- Can be comboed from Chain Combo or Blade Stop for reduced sphere cost.

### Quest Skills

**Ki Translation** (rA: 1015) — SP: 40. Transfer 1 spirit sphere to party member.

**Ki Explosion** (rA: 1016) — SP: 40, HP: 200. 300% ATK AoE knockback + stun.

---

# Transcendent Classes

Transcendent (Trans) classes are reborn versions of 2nd classes. They retain ALL skills from their 1st and 2nd class and gain additional exclusive skills. They also receive +25% HP/SP from transcendence.

---

## Lord Knight

*Transcendent of Knight. Inherits all Swordsman + Knight skills.*

### Aura Blade
| Field | Value |
|-------|-------|
| ID | rA: 355 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| Duration | 20-100s (20*Lv) |
| Description | Adds bonus ATK that bypasses DEF. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 18 | 26 | 34 | 42 | 50 |
| +ATK | 20 | 40 | 60 | 80 | 100 |

### Parry (Parrying)
| Field | Value |
|-------|-------|
| ID | rA: 356 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| SP Cost | 50 |
| Requirement | Two-Handed Sword |
| Duration | 15-60s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Block% | 23 | 26 | 29 | 32 | 35 | 38 | 41 | 44 | 47 | 50 |

### Concentration (Spear Dynamo)
| Field | Value |
|-------|-------|
| ID | rA: 357 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| Duration | 25-85s |
| Description | +ATK% and +HIT%, but -DEF%. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 14 | 18 | 22 | 26 | 30 |
| ATK/HIT +% | 5 | 10 | 15 | 20 | 25 |
| DEF -% | 5 | 10 | 15 | 20 | 25 |

### Tension Relax (Relax)
| Field | Value |
|-------|-------|
| ID | rA: 358 |
| Type | Supportive |
| Max Level | 1 |
| SP Cost | 15 |
| Description | Triples HP regen. Must sit. Canceled by any action. |

### Berserk (Frenzy)
| Field | Value |
|-------|-------|
| ID | rA: 359 |
| Type | Supportive |
| Max Level | 1 |
| SP Cost | 200 |
| Duration | 300s |
| Description | All SP consumed. MaxHP x3. ATK x2. DEF halved. FLEE = 1. Cannot use skills/items/chat. HP drains 5%/15s. Ends when HP drops below 100. |

### Spiral Pierce (Clashing Spiral)
| Field | Value |
|-------|-------|
| ID | rA: 397 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 4 cells |
| Prerequisites | Pierce Lv5, Spear Stab Lv5, Spear Boomerang Lv3 |
| Cast Time | 0.3-1.5s |
| ACD | 1-2.5s |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 18 | 21 | 24 | 27 | 30 |
| ATK% | 150 | 200 | 250 | 300 | 350 |

- 5 hits. Damage affected by weapon weight. Requires spear.

### Head Crush (Traumatic Blow)
| Field | Value |
|-------|-------|
| ID | rA: 398 |
| Type | Offensive |
| Max Level | 5 |
| SP Cost | 23 |
| Description | Chance to cause Bleeding status. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 |
| Bleed% | 20 | 30 | 40 | 50 | 60 |

### Joint Beat (Vital Strike)
| Field | Value |
|-------|-------|
| ID | rA: 399 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Prerequisites | Cavalier Mastery Lv3, Head Crush Lv3 |
| Description | Random status debuff (ankle break, wrist break, knee break, shoulder break, waist break, neck break). |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 12 | 12 | 14 | 14 | 16 | 16 | 18 | 18 | 20 | 20 |
| ATK% | 60 | 70 | 80 | 90 | 100 | 110 | 120 | 130 | 140 | 150 |

---

## Paladin

*Transcendent of Crusader. Inherits all Swordsman + Crusader skills.*

### Gloria Domini (Pressure)
| Field | Value |
|-------|-------|
| ID | rA: 367 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 9 cells |
| Element | Holy |
| Prerequisites | Grand Cross Lv5, Faith Lv5 |
| Cast Time | 1-3s |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 30 | 35 | 40 | 45 | 50 |

- Fixed damage based on (ATK + MATK) * SkillLv. Ignores DEF/MDEF.
- Target loses (2*Lv)% SP.

### Martyr's Reckoning (Sacrifice)
| Field | Value |
|-------|-------|
| ID | rA: 368 |
| Type | Supportive |
| Max Level | 5 |
| SP Cost | 100 |
| Duration | 30s (or 5 hits) |
| Prerequisites | Devotion Lv3, Endure Lv1 |
| Description | Each attack drains 9% MaxHP but deals massive damage. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK Multiplier | 1.0x | 1.1x | 1.2x | 1.3x | 1.4x |

### Battle Chant (Gospel)
| Field | Value |
|-------|-------|
| ID | rA: 369 |
| Type | Supportive |
| Max Level | 10 |
| Target | AoE (self-centered) |
| Prerequisites | Cure Lv1, Divine Protection Lv3 |

| Level | 1-5 | 6-10 |
|-------|------|------|
| SP | 80 | 100 |
| Duration(s) | 60 | 60 |

- Random positive effects on allies, random negative effects on enemies.

### Shield Chain (Rapid Smiting)
| Field | Value |
|-------|-------|
| ID | rA: 480 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 4 cells |
| Prerequisites | Shield Boomerang Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 28 | 31 | 34 | 37 | 40 |
| ATK% | 500 | 700 | 900 | 1100 | 1300 |
| Hits | 5 | 5 | 5 | 5 | 5 |

- Damage based on shield + ATK.

---

## High Wizard

*Transcendent of Wizard. Inherits all Mage + Wizard skills.*

### Soul Drain
| Field | Value |
|-------|-------|
| ID | rA: 364 |
| Type | Passive |
| Max Level | 10 |
| Description | +2% MaxSP/level. Recover SP when killing monsters with single-target magic. |

### Magic Crasher (Stave Crasher)
| Field | Value |
|-------|-------|
| ID | rA: 365 |
| Type | Offensive |
| Max Level | 1 |
| Target | Single Enemy |
| Range | 9 cells |
| SP Cost | 8 |
| Element | Neutral |
| Description | Physical attack using MATK instead of ATK. Reduced by enemy's physical DEF. |

### Mystical Amplification
| Field | Value |
|-------|-------|
| ID | rA: 366 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Duration | Next magic skill |
| Description | Amplifies the MATK of the next spell cast. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| MATK +% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

### Napalm Vulcan
| Field | Value |
|-------|-------|
| ID | rA: 400 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single (3x3 splash) |
| Element | Ghost |
| Prerequisites | Napalm Beat Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 30 | 40 | 50 | 60 | 70 |
| MATK% | 70 | 140 | 210 | 280 | 350 |
| Hits | 1 | 2 | 3 | 4 | 5 |

### Ganbantein
| Field | Value |
|-------|-------|
| ID | rA: 483 |
| Type | Active |
| Max Level | 1 |
| Target | Ground (3x3) |
| SP Cost | 40 |
| Catalyst | 1 Blue Gemstone + 1 Yellow Gemstone |
| Description | 80% chance to cancel all ground-targeted spells in area. |

### Gravitational Field
| Field | Value |
|-------|-------|
| ID | rA: 484 |
| Type | Offensive |
| Max Level | 5 |
| Target | Ground (5x5) |
| Element | Earth |
| Catalyst | 1 Blue Gemstone |
| Prerequisites | Mystical Amplification Lv10, Quagmire Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 60 | 70 | 80 | 90 | 100 |
| MATK% per hit | 100 | 200 | 300 | 400 | 500 |
| Hits | 2 | 4 | 6 | 8 | 10 |
| Duration(s) | 5 | 6 | 7 | 8 | 9 |

- Damage ignores elemental modifiers. Reduces ASPD of enemies in area.

---

## Scholar (Professor)

*Transcendent of Sage. Inherits all Mage + Sage skills.*

### Health Conversion (Indulge)
| Field | Value |
|-------|-------|
| ID | rA: 373 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| Description | Consume 10% MaxHP, recover SP. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Recovered | 10% | 20% | 30% | 40% | 50% |

(Of the HP consumed, converted to SP at percentage rate.)

### Soul Change (Soul Exhale)
| Field | Value |
|-------|-------|
| ID | rA: 374 |
| Type | Active |
| Max Level | 1 |
| Target | Single (ally or enemy) |
| SP Cost | 5 |
| Description | Swap your SP with target's SP. |

### Soul Burn (Soul Siphon)
| Field | Value |
|-------|-------|
| ID | rA: 375 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Description | Drain target's SP. On success, you gain it. On fail, you lose double SP. |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 80 | 90 | 100 | 110 | 120 |
| Success% | 40 | 50 | 60 | 70 | 70 |

### Mind Breaker
| Field | Value |
|-------|-------|
| ID | rA: 402 |
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 12 | 15 | 18 | 21 | 24 |
| MATK+ | 20% | 40% | 60% | 80% | 100% |
| Soft MDEF- | 12 | 24 | 36 | 48 | 60 |

### Memorize (Foresight)
| Field | Value |
|-------|-------|
| ID | rA: 403 |
| Type | Supportive |
| Max Level | 1 |
| Target | Self |
| SP Cost | 1 |
| Cast Time | 5s |
| Description | Next 5 spells have cast time reduced by 50%. |

### Wall of Fog (Blinding Mist)
| Field | Value |
|-------|-------|
| ID | rA: 404 |
| Type | Active |
| Max Level | 1 |
| Target | Ground (5x3) |
| SP Cost | 25 |
| Duration | 20s |
| Description | Blind enemies in area. Ranged attacks have -75% damage and 75% miss chance in fog. |

### Spider Web (Fiber Lock)
| Field | Value |
|-------|-------|
| ID | rA: 405 |
| Type | Active |
| Max Level | 1 |
| Target | Single Enemy |
| SP Cost | 30 |
| Description | Immobilize target for 8s (4s vs players). Halves FLEE. Fire damage breaks web (double damage). |

### Double Casting (Double Bolt)
| Field | Value |
|-------|-------|
| ID | rA: 482 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| Duration | 90s |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 40 | 45 | 50 | 55 | 60 |
| Double-cast% | 40 | 50 | 60 | 70 | 80 |

- Bolt spells (Fire/Cold/Lightning Bolt) have a chance to cast twice per cast.

---

## Sniper

*Transcendent of Hunter. Inherits all Archer + Hunter skills.*

### True Sight (Falcon Eyes)
| Field | Value |
|-------|-------|
| ID | rA: 380 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Prerequisites | Improve Concentration Lv1 |
| Duration | 30-120s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 65 |
| All Stats+ | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 |
| ATK+ | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| HIT+ | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| CRIT+ | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |

### Falcon Assault
| Field | Value |
|-------|-------|
| ID | rA: 381 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 9 cells |
| Prerequisites | Blitz Beat Lv5, Steel Crow Lv3, True Sight Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 30 | 34 | 38 | 42 | 46 |

- **Damage:** ((DEX * 0.8 + INT * 0.2) * Level * 3 + (Blitz Beat Damage * 3))
- Ignores DEF. Requires Falcon.

### Sharp Shooting (Focused Arrow Strike)
| Field | Value |
|-------|-------|
| ID | rA: 382 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single (12x3 line AoE) |
| Range | 9 cells |
| Prerequisites | Double Strafe Lv5 |
| Cast Time | 1.5-3.5s |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 18 | 21 | 24 | 27 | 30 |
| ATK% | 200 | 250 | 300 | 350 | 400 |

- Hits all enemies in a narrow line toward target. Can critical.

### Wind Walker
| Field | Value |
|-------|-------|
| ID | rA: 383 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self (party-wide) |
| Prerequisites | Improve Concentration Lv9 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 46 | 52 | 58 | 64 | 70 | 76 | 82 | 88 | 94 | 100 |
| FLEE+ | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| Move Speed +% | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 | 25 |
| Duration(s) | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 370 | 400 |

---

## Clown (Minstrel)

*Transcendent of Bard. Inherits all Archer + Bard skills.*

### Arrow Vulcan
| Field | Value |
|-------|-------|
| ID | rA: 394 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Prerequisites | Musical Strike Lv3, Music Lessons Lv5 |
| Cast Time | 3-4.5s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| ATK% | 600 | 700 | 800 | 900 | 1000 | 1100 | 1100 | 1200 | 1300 | 1500 |

- Consumes 1 arrow. Multiple hits.

### Marionette Control
| Field | Value |
|-------|-------|
| ID | rA: 396 |
| Type | Supportive |
| Max Level | 1 |
| Target | Single Party Member |
| SP Cost | 100 |
| Duration | Until canceled or target moves out of range |
| Description | Transfer half of caster's base stats to target (capped at 99). Both immobilized during effect. |

### Tarot Card of Fate
| Field | Value |
|-------|-------|
| ID | rA: 489 |
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| SP Cost | 40 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Success% | 8 | 16 | 24 | 32 | 40 |

- Random 1 of 14 effects (The Fool, Magician, High Priestess, Chariot, Strength, Lovers, Wheel of Fortune, Hanged Man, Death, Temperance, Devil, Tower, Star, Sun).

### Longing for Freedom
| Field | Value |
|-------|-------|
| ID | rA: 487 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 15 |
| Description | Move while performing ensemble skills. Speed: 60-100%. |

### Wand of Hermode
| Field | Value |
|-------|-------|
| ID | rA: 488 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 20-60 |
| Description | WoE skill. Blocks magic attacks near warp portals. Strips buffs from affected. |

### Sheltering Bliss (Moonlit Water Mill)
| Field | Value |
|-------|-------|
| ID | rA: 395 |
| Type | Ensemble |
| Max Level | 5 |
| SP Cost | 30-70 |
| AoE | 9x9 |
| Description | Prevents entry into area. Does not block ranged/magic. |

---

## Gypsy

*Transcendent of Dancer. Inherits all Archer + Dancer skills.*

All Gypsy transcendent skills are **identical** to Clown skills:
- **Arrow Vulcan** (rA: 394) — Same as Clown.
- **Marionette Control** (rA: 396) — Same as Clown.
- **Tarot Card of Fate** (rA: 489) — Same as Clown.
- **Longing for Freedom** (rA: 487) — Same as Clown.
- **Wand of Hermode** (rA: 488) — Same as Clown.
- **Sheltering Bliss** (rA: 395) — Same as Clown.

---

## Assassin Cross

*Transcendent of Assassin. Inherits all Thief + Assassin skills.*

### Advanced Katar Mastery
| Field | Value |
|-------|-------|
| ID | rA: 376 |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Katar Mastery Lv7 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Katar ATK +% | 12 | 14 | 16 | 18 | 20 |

### Enchant Deadly Poison (EDP)
| Field | Value |
|-------|-------|
| ID | rA: 378 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| Prerequisites | Create Deadly Poison Lv1, Enchant Poison Lv6 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 60 | 70 | 80 | 90 | 100 |
| Duration(s) | 40 | 45 | 50 | 55 | 60 |
| ATK Multiplier | 2x | 2.5x | 3x | 3.5x | 4x |

- Consumes 1 Poison Bottle.
- Chance to inflict deadly poison status on each hit.

### Soul Destroyer (Soul Breaker)
| Field | Value |
|-------|-------|
| ID | rA: 379 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Prerequisites | Enchant Poison Lv6, Cloaking Lv3 |
| Cast Time | 0.5-1.0s |
| ACD | 1s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 20 | 20 | 20 | 20 | 20 | 25 | 25 | 25 | 25 | 30 |
| ATK% | 150 | 300 | 450 | 600 | 750 | 900 | 1050 | 1200 | 1350 | 1500 |

- **Formula:** `(ATK% + (INT * 5 * Lv) + random(500,1000))`
- Ranged physical + magical hybrid.

### Meteor Assault
| Field | Value |
|-------|-------|
| ID | rA: 406 |
| Type | Offensive |
| Max Level | 10 |
| Target | AoE (5x5 self-centered) |
| Prerequisites | Sonic Blow Lv10, Katar Mastery Lv5 |
| Cast Time | 0.5s |
| ACD | 0.5s |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 |
| ATK% | 80 | 160 | 240 | 320 | 400 | 480 | 560 | 640 | 720 | 800 |

- Status effects: Stun, Blind, Bleed (chance scales with level).

### Create Deadly Poison
| Field | Value |
|-------|-------|
| ID | rA: 407 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 50 |
| Description | Create 1 Poison Bottle. Success ~20% + 0.4*DEX + 0.2*LUK. Failure damages caster. |

---

## Stalker

*Transcendent of Rogue. Inherits all Thief + Rogue skills.*

### Chase Walk (Stealth)
| Field | Value |
|-------|-------|
| ID | rA: 389 |
| Type | Active |
| Max Level | 5 |
| SP Cost | 10 |
| Prerequisites | Tunnel Drive Lv3, Hiding Lv5 |
| Description | Special hide that cannot be detected by detection skills. Movement while hidden. STR bonus on reveal. |

### Reject Sword (Counter Instinct)
| Field | Value |
|-------|-------|
| ID | rA: 390 |
| Type | Active |
| Max Level | 5 |
| Target | Self |
| SP Cost | 10-30 |
| Description | Block up to 3 melee attacks. Deflected damage halved, reflected back at attacker. |

### Preserve
| Field | Value |
|-------|-------|
| ID | rA: 475 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 30 |
| Duration | 10 minutes |
| Description | Prevents Plagiarism from overwriting your copied skill. |

### Full Strip (Full Divestment)
| Field | Value |
|-------|-------|
| ID | rA: 476 |
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| Prerequisites | Strip Weapon Lv5, Strip Shield Lv5, Strip Armor Lv5, Strip Helm Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 22 | 24 | 26 | 28 | 30 |
| Success% | 12 | 14 | 16 | 18 | 20 |

- Strips ALL equipment simultaneously.

---

## Whitesmith (Mastersmith)

*Transcendent of Blacksmith. Inherits all Merchant + Blacksmith skills.*

### Melt Down (Shattering Strike)
| Field | Value |
|-------|-------|
| ID | rA: 384 |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Description | Chance to break enemy weapon/armor on each hit. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 50 | 50 | 60 | 60 | 70 | 70 | 80 | 80 | 90 | 90 |
| Weapon Break% | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Armor Break% | 0.7 | 1.4 | 2.1 | 2.8 | 3.5 | 4.2 | 4.9 | 5.6 | 6.3 | 7.0 |
| Duration(s) | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 |

### Cart Boost
| Field | Value |
|-------|-------|
| ID | rA: 387 |
| Type | Supportive |
| Max Level | 1 |
| SP Cost | 20 |
| Duration | 60s |
| Requirement | Pushcart equipped |
| Description | +100% move speed while pushing cart. |

### Cart Termination (High Speed Cart Ram)
| Field | Value |
|-------|-------|
| ID | rA: 485 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee |
| Prerequisites | Cart Boost Lv1, Mammonite Lv10, Power-Thrust Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 15 | 15 | 15 | 15 | 15 | 15 | 15 | 15 | 15 | 15 |
| Stun% | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

- Damage scales with cart weight: heavier cart = more damage.

### Maximum Over Thrust (Maximum Power-Thrust)
| Field | Value |
|-------|-------|
| ID | rA: 486 |
| Type | Supportive |
| Max Level | 5 |
| Target | Self |
| SP Cost | 15 |
| Prerequisites | Power-Thrust Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK +% | 20 | 40 | 60 | 80 | 100 |
| Zeny Cost | 3000 | 3500 | 4000 | 4500 | 5000 |
| Duration(s) | 60 | 60 | 60 | 60 | 60 |

- Slight chance to break own weapon on attack.
- Self-only (unlike Power-Thrust which is party-wide).

### Weapon Refine (Upgrade Weapon)
| Field | Value |
|-------|-------|
| ID | rA: 477 |
| Type | Active |
| Max Level | 10 |
| SP Cost | 30 |
| Description | Refine weapons with higher success rate than NPC. |

---

## Creator (Biochemist)

*Transcendent of Alchemist. Inherits all Merchant + Alchemist skills.*

### Acid Demonstration (Acid Bomb)
| Field | Value |
|-------|-------|
| ID | rA: 490 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells |
| Catalyst | 1 Acid Bottle + 1 Bottle Grenade |
| Prerequisites | Acid Terror Lv5, Demonstration Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 30 | 50 |
| Hits | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |

- **Damage formula:** `(0.7 * Caster's INT^2 * Target's VIT) / (Lv * 50)`
- Ignores DEF. Chance to break weapon and armor. Always hits.

### Slim Potion Pitcher (Aid Condensed Potion)
| Field | Value |
|-------|-------|
| ID | rA: 478 |
| Type | Supportive |
| Max Level | 10 |
| Target | Ground (7x7) |
| SP Cost | 30 |
| Description | Throw condensed potions to heal all party/guild members in area. |

### Full Chemical Protection
| Field | Value |
|-------|-------|
| ID | rA: 479 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Ally |
| SP Cost | 40 |
| Catalyst | 1 Glistening Coat |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration(s) | 120 | 240 | 360 | 480 | 600 |

- Protects all 4 equipment slots (weapon, shield, armor, helm) from break/strip.

### Plant Cultivation
| Field | Value |
|-------|-------|
| ID | rA: 491 |
| Type | Active |
| Max Level | 2 |
| SP Cost | 10 |
| Lv1 | Summon random mushroom (50% success) |
| Lv2 | Summon random plant (50% success) |

---

## High Priest

*Transcendent of Priest. Inherits all Acolyte + Priest skills.*

### Assumptio
| Field | Value |
|-------|-------|
| ID | rA: 361 |
| Type | Supportive |
| Max Level | 5 |
| Target | Single (self/ally) |
| Cast Time | 1-3s |
| Prerequisites | Impositio Manus Lv3, Increase SP Recovery Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 20 | 30 | 40 | 50 | 60 |
| Duration(s) | 20 | 40 | 60 | 80 | 100 |

- Halves all incoming damage (PvM). Reduces by 1/3 (PvP). Disabled in WoE.

### Basilica
| Field | Value |
|-------|-------|
| ID | rA: 362 |
| Type | Supportive |
| Max Level | 5 |
| Target | Ground (5x5 self-centered) |
| Catalyst | 1 Blue Gemstone + 1 Yellow Gemstone + 1 Holy Water + 1 Red Gemstone |
| Prerequisites | Gloria Lv2 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 40 | 50 | 60 | 70 | 80 |
| Duration(s) | 20 | 25 | 30 | 35 | 40 |

- Characters inside cannot attack or be attacked. Caster must remain stationary.

### Meditatio
| Field | Value |
|-------|-------|
| ID | rA: 363 |
| Type | Passive |
| Max Level | 10 |
| Description | Increases Heal effectiveness, MaxSP, and SP regen. |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Heal +% | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| MaxSP +% | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| SP Regen +% | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

### Mana Recharge (Spiritual Thrift)
| Field | Value |
|-------|-------|
| ID | rA: 481 |
| Type | Passive |
| Max Level | 5 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost Reduction | 4% | 8% | 12% | 16% | 20% |

---

## Champion

*Transcendent of Monk. Inherits all Acolyte + Monk skills.*

### Palm Push Strike (Raging Palm Strike)
| Field | Value |
|-------|-------|
| ID | rA: 370 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee |
| Prerequisites | Iron Fists Lv7, Fury Lv5 |
| Requirement | Fury state active |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 2 | 4 | 6 | 8 | 10 |
| ATK% | 300 | 400 | 500 | 600 | 700 |

- Knockback 3 cells. Can combo from Raging Thrust.

### Tiger Knuckle Fist (Glacier Fist)
| Field | Value |
|-------|-------|
| ID | rA: 371 |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Prerequisites | Palm Push Strike Lv3, Chain Combo Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 4 | 6 | 8 | 10 | 12 |
| ATK% | 140 | 360 | 580 | 800 | 1020 |

- Chance to immobilize for 1 second.

### Chain Crush Combo
| Field | Value |
|-------|-------|
| ID | rA: 372 |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Cost | 1 Spirit Sphere |
| Prerequisites | Tiger Knuckle Fist Lv3 |
| Trigger | Must follow Tiger Knuckle Fist or Raging Thrust |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 |
| ATK% | 200 | 400 | 600 | 800 | 1000 | 1100 | 1200 | 1300 | 1400 | 1500 |

- Can combo into Guillotine Fist with reduced sphere cost.

### Zen (Dangerous Soul Collect)
| Field | Value |
|-------|-------|
| ID | rA: 401 |
| Type | Active |
| Max Level | 1 |
| SP Cost | 20 |
| Description | Instantly summon 5 Spirit Spheres at once. |

---

## Implementation Notes

### Data Structure (Server)

Skills are defined in `server/src/ro_skill_data.js` (1st class) and `server/src/ro_skill_data_2nd.js` (2nd class).

```javascript
{
  id: 103,                    // Internal skill ID
  name: 'bash',               // Machine name (snake_case)
  displayName: 'Bash',        // Human-readable name
  classId: 'swordsman',       // Class that learns this skill
  maxLevel: 10,
  type: 'active',             // passive | active | toggle | offensive
  targetType: 'single',       // none | self | single | ground | aoe
  element: 'neutral',         // neutral | fire | water | wind | earth | holy | ghost | poison
  range: 150,                 // UE units (1 RO cell ~ 50 UE units)
  description: '...',
  icon: 'bash',
  treeRow: 0,                 // Skill tree UI position
  treeCol: 1,
  prerequisites: [{ skillId: X, level: Y }],
  levels: [
    { level: 1, spCost: 8, castTime: 0, afterCastDelay: 0, cooldown: 700, effectValue: 130, duration: 0 },
    // ... one entry per level
  ]
}
```

### Database Schema

Skills learned are stored per character:

```sql
-- character_skills table
character_id  INTEGER REFERENCES characters(id),
skill_id      INTEGER NOT NULL,
skill_level   INTEGER NOT NULL DEFAULT 1,
PRIMARY KEY (character_id, skill_id)
```

### UE5 Targeting (Client)

The client uses three targeting modes based on `targetType`:

| targetType | Client Behavior |
|-----------|----------------|
| `self` | Immediate cast, no targeting needed |
| `single` | Enter SingleTarget mode: click enemy to cast |
| `ground` | Enter GroundTarget mode: click ground cell, shows AoE indicator |
| `aoe` | Immediate cast centered on caster |

Relevant C++ files:
- `SabriMMOCharacter.cpp` — `UseSkill()`, `UseSkillOnTarget()`, `UseSkillOnGround()`
- `SkillVFXSubsystem.cpp` — Visual effect dispatch
- `SkillVFXData.cpp` — Per-skill VFX config

### Cast Time Implementation

Server applies cast time in milliseconds:
```javascript
// In server combat handler
const finalCastTime = baseCastTime * Math.max(0, (150 - player.dex) / 150);
```

### Key Formulas Reference

| Formula | Expression |
|---------|-----------|
| Bolt Damage | `NumBolts * MATK` |
| Heal | `floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)` |
| Cast Time Reduction | `BaseCast * (150 - DEX) / 150` |
| Asura Strike | `ATK * (8 + SP/10) + LevelBonus` |
| Acid Demonstration | `0.7 * INT^2 * TargetVIT / (SkillLv * 50)` |
| Provoke DEF Reduction | `(5 + SkillLv * 5)%` |
| Provoke ATK Increase | `(2 + SkillLv * 3)%` |
| Frost Diver Freeze Duration | `SkillLv * 3 seconds` |

### ID Ranges (Internal)

| Range | Class |
|-------|-------|
| 1-10 | Novice |
| 100-119 | Swordsman |
| 200-229 | Mage |
| 300-319 | Archer |
| 400-429 | Acolyte |
| 500-519 | Thief |
| 600-619 | Merchant |
| 700+ | 2nd classes (see ro_skill_data_2nd.js) |

