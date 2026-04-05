# Rogue Class — Complete Pre-Renewal Research & Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Rogue_Skills_Audit](Rogue_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Sources:** iRO Wiki Classic, rAthena pre-renewal DB, RateMyServer, rAthena GitHub, DarkHikari's guide
**Base Class:** Thief (inherits all 10 Thief skills: IDs 500-509)
**Transcendent:** Stalker (future phase)

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Complete Skill List](#2-complete-skill-list)
3. [Skill Tree (Prerequisites)](#3-skill-tree-prerequisites)
4. [Detailed Skill Specifications](#4-detailed-skill-specifications)
5. [Plagiarism System Architecture](#5-plagiarism-system-architecture)
6. [Strip Equipment System Architecture](#6-strip-equipment-system-architecture)
7. [New Systems Required](#7-new-systems-required)
8. [Existing Code Audit](#8-existing-code-audit)
9. [Implementation Priority](#9-implementation-priority)
10. [Integration Points](#10-integration-points)
11. [Rogue-Specific Constants and Data Tables](#11-rogue-specific-constants-and-data-tables)
12. [Sources](#12-sources)

---

## 1. Class Overview

Rogues are the 2-2 branch of the Thief class tree. They are versatile combatants who excel at:

- **Plagiarism**: Copying and using the last skill that damaged them (the class's signature mechanic)
- **Equipment Stripping**: Forcibly removing enemy equipment (weapon/shield/armor/helm) in PvP
- **Hybrid Weapons**: Capable with daggers, 1H swords, AND bows (unique dual-weapon-type class)
- **Stealth Combat**: Raid from Hiding for AoE surprise attacks
- **Economy**: Auto-steal, Zeny theft, NPC discounts

**Class progression chain:** `['novice', 'thief', 'rogue']`
**Job change requirement:** Thief Base Lv 40+, Job Lv 40+
**Total skill points available:** 49 (job levels 1-49, 2nd class)

### Rogue Stat Builds (Common)

| Build | STR | AGI | VIT | INT | DEX | LUK | Focus |
|-------|-----|-----|-----|-----|-----|-----|-------|
| Dagger | 80+ | 70+ | 30 | 10 | 40 | 1 | Backstab, melee DPS |
| Bow | 30 | 70+ | 20 | 10 | 80+ | 1 | Double Strafe, ranged |
| Strip PvP | 30 | 50 | 60+ | 30 | 80+ | 1 | Divest skills, survivability |
| Plagiarism | Varies | Varies | 50+ | 50+ | 80+ | 1 | Copied skill depends on build |

---

## 2. Complete Skill List

### Inherited from Thief (IDs 500-509)

| ID | Name | Type | Already Implemented |
|----|------|------|---------------------|
| 500 | Double Attack | Passive | Yes |
| 501 | Improve Dodge | Passive | Yes |
| 502 | Steal | Active | Yes (gaps noted in Thief audit) |
| 503 | Hiding | Toggle | Yes (gaps noted in Thief audit) |
| 504 | Envenom | Active | Yes (needs formula fix) |
| 505 | Detoxify | Active | Yes |
| 506 | Sand Attack | Active | Yes |
| 507 | Back Slide | Active | Yes |
| 508 | Pick Stone | Active | Yes (stub) |
| 509 | Throw Stone | Active | Yes (stub) |

### Rogue Own Skills (IDs 1700-1718)

| ID | Name | Type | Max Lv | New System Required |
|----|------|------|--------|---------------------|
| 1700 | Snatcher (Gank) | Passive | 10 | Auto-steal on attack |
| 1701 | Back Stab | Active | 10 | Behind-target positioning, always-hit |
| 1702 | Tunnel Drive (Stalk) | Passive | 5 | Hidden movement speed |
| 1703 | Raid (Sightless Mind) | Active | 5 | AoE from Hiding, stun/blind |
| 1704 | Intimidate (Snatch) | Active | 5 | Attack + teleport both |
| 1705 | Sword Mastery (Rogue) | Passive | 10 | Passive ATK bonus (shared skill) |
| 1706 | Vulture's Eye (Rogue) | Passive | 10 | Passive range/HIT bonus (shared skill) |
| 1707 | Double Strafe (Rogue) | Active | 10 | Bow skill (shared skill) |
| 1708 | Remove Trap (Rogue) | Active | 1 | Trap removal (shared skill) |
| 1709 | Steal Coin (Mug) | Active | 10 | Zeny theft system |
| 1710 | Divest Helm | Active | 5 | **Strip equipment system** |
| 1711 | Divest Shield | Active | 5 | **Strip equipment system** |
| 1712 | Divest Armor | Active | 5 | **Strip equipment system** |
| 1713 | Divest Weapon | Active | 5 | **Strip equipment system** |
| 1714 | Plagiarism | Passive | 10 | **Plagiarism system (COMPLEX)** |
| 1715 | Gangster's Paradise | Passive | 1 | Sitting group AI immunity |
| 1716 | Compulsion Discount | Passive | 5 | NPC shop price reduction |
| 1717 | Scribble (Graffiti) | Active | 1 | Quest skill, cosmetic |
| 1718 | Close Confine | Active | 1 | Quest skill, movement lock |

**Total: 19 Rogue skills + 10 inherited Thief skills = 29 skills**

---

## 3. Skill Tree (Prerequisites)

```
Thief Prerequisites:
  Steal (502) Lv1 -----> Snatcher (1700) Lv4 -----> Steal Coin (1709) Lv2 -----> Divest Helm (1710) Lv5 --->
  Hiding (503) Lv1 ----> Tunnel Drive (1702) Lv2 -+                               Divest Shield (1711) Lv5 --->
                                                   |                                 Divest Armor (1712) Lv5 --->
                                                   |                                   Divest Weapon (1713)
                                                   |
                                                   +-> Back Stab (1701) Lv2 --+---> Raid (1703) Lv5 --->
                                                                               |      Intimidate (1704) Lv5 --->
                                                   Steal Coin (1709) Lv4 --+   |        Plagiarism (1714)
                                                                           |   |
                                                   Back Stab (1701) Lv4 --+---+

                                                   Raid (1703) Lv1 -----> Gangster's Paradise (1715) Lv1 --->
                                                                            Compulsion Discount (1716)

  (No prerequisites):                              Sword Mastery (1705)
  Vulture's Eye (1706) Lv10 -----> Double Strafe (1707) Lv5 -----> Remove Trap (1708)

  Quest Skills (no prereqs in tree):               Scribble (1717), Close Confine (1718)
```

### Skill Tree Grid Layout (treeRow x treeCol)

| Row\Col | 0 | 1 | 2 | 3 |
|---------|---|---|---|---|
| 0 | Snatcher | -- | Sword Mastery | Vulture's Eye |
| 1 | Back Stab | Steal Coin | -- | Double Strafe |
| 2 | Tunnel Drive | Divest Helm | -- | Remove Trap |
| 3 | Raid | Divest Shield | -- | Plagiarism |
| 4 | Intimidate | Divest Armor | Gangster's Paradise | -- |
| 5 | -- | Divest Weapon | Compulsion Discount | -- |
| 6 | Scribble (Q) | Close Confine (Q) | -- | -- |

---

## 4. Detailed Skill Specifications

---

### 4.1 Snatcher / Gank (ID 1700) -- Passive

**rAthena ID:** 210 (RG_SNATCHER)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisite | Steal (502) Lv1 |
| Effect | Auto-Steal on each physical melee attack |
| Steal success | Uses Steal's formula when triggered |
| Melee only | Does NOT trigger on ranged attacks (bow) or skills |

**Trigger Chance Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Chance (%) | 7 | 8 | 10 | 11 | 13 | 14 | 16 | 17 | 19 | 20 |

**Formula (rAthena):** `triggerChance = 5 + (level * 1.5)` rounded, but per iRO Wiki the values are fixed as above.

**Mechanic:**
1. On each physical melee auto-attack hit, roll `triggerChance`
2. If triggered, execute a Steal attempt using the Thief `Steal` skill's formula
3. The stolen item is determined by Steal's formula (DEX/monster DEX/drop table)
4. Stolen item goes to inventory (weight check applies)
5. Does NOT consume SP
6. Can steal once per monster (uses same `enemy.stolenBy` tracker as Steal)

**Implementation:** Integrate into combat tick auto-attack pipeline. After damage, if player is Rogue and has Snatcher, roll the chance. On success, run Steal's item acquisition logic.

---

### 4.2 Back Stab (ID 1701) -- Active

**rAthena ID:** 212 (RG_BACKSTAP)

| Property | Value |
|----------|-------|
| Type | Active, Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element (uses equipped weapon's element) |
| SP Cost | 16 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0.5 seconds (ASPD-based in iRO Classic) |
| Cooldown | 0 |
| Prerequisites | Steal Coin (1709) Lv4 |
| Always hits | Yes -- never misses (bypasses FLEE/Perfect Dodge) |
| Position | Pre-renewal: must be behind target (caster facing target's back) |
| Can use from Hiding | Yes (pre-renewal) -- cancels Hiding on use |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 340 | 380 | 420 | 460 | 500 | 540 | 580 | 620 | 660 | 700 |

**Formula:** `ATK% = 300 + level * 40`

**Weapon Modifiers:**
- **Dagger:** Deals 2 hits (each hit at full ATK%)
- **Bow:** Damage halved (50% penalty)
- **Other weapons (1H sword, etc.):** 1 hit at full damage

**Special Mechanics:**
1. Teleports caster behind the target (server moves caster position)
2. Target is turned to face the caster after being hit
3. Cards and element modifiers apply normally
4. Cancels Hiding status if used while hidden (pre-renewal allows casting from Hide)
5. Does NOT crit (skill, not auto-attack)
6. Blocked by Cicada Skin Shed, Kaupe, Illusionary Shadow (Ninja/Soul Linker skills)

**Implementation Notes:**
- Need `forceHit: true` in damage calculation (bypasses FLEE)
- Dagger check: `weaponType === 'dagger'` -> 2 hits, each at full effectValue%
- Bow check: `weaponType === 'bow'` -> 1 hit at effectValue/2 %
- Position check: for PvE, simplified -- always allow (monsters don't have meaningful facing). For PvP, compare attacker facing vs target position.
- Cancel hiding: check `hasBuff(player, 'hiding')`, remove it, broadcast `skill:buff_removed`

---

### 4.3 Tunnel Drive / Stalk (ID 1702) -- Passive

**rAthena ID:** 213 (RG_TUNNELDRIVE)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisite | Hiding (503) Lv1 |
| Effect | Enables movement while in Hiding status |

**Movement Speed While Hidden:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Speed % | 26 | 32 | 38 | 44 | 50 |

**Mechanic:**
- Without Tunnel Drive, Hiding blocks ALL movement (as per Thief Hiding)
- With Tunnel Drive, movement is allowed but at reduced speed
- Speed penalty is `(100 - tunnelDriveSpeed)%` of normal speed
- Hidden status still active during movement (still invisible, enemies cannot target unless detector)
- SP drain from Hiding still applies during movement

**Implementation Notes:**
- When checking Hiding movement block in `player:position` handler, check if player has Tunnel Drive passive skill
- If Tunnel Drive learned, allow movement but cap speed (server-side velocity limit)
- Speed implementation: accept position updates but with a maximum distance per tick check scaled by `tunnelDriveSpeed/100`

---

### 4.4 Raid / Sightless Mind (ID 1703) -- Active

**rAthena ID:** 214 (RG_RAID)

| Property | Value |
|----------|-------|
| Type | Active, Offensive AoE |
| Max Level | 5 |
| Target | Self-centered AoE |
| AoE | 3x3 cells (pre-renewal -- NOT 5x5 Renewal) |
| Range | 0 (self-centered) |
| Element | Weapon element |
| SP Cost | 20 (pre-renewal, all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 (ASPD-based) |
| Cooldown | 0 |
| Prerequisites | Back Stab (1701) Lv2, Tunnel Drive (1702) Lv2 |
| Requirement | Must be in Hiding status |
| Effect | Cancels Hiding on use |

**Damage Per Level (Pre-Renewal):**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 |
| Stun/Blind % | 13 | 16 | 19 | 22 | 25 |

**Formula:** `ATK% = 100 + level * 40`, `StatusChance = 10 + level * 3`

**Special Mechanics:**
1. MUST be in Hiding to use -- emit `skill:error` if not hidden
2. Cancels Hiding immediately upon use
3. Deals physical weapon damage to all enemies in 3x3 around caster
4. Each target independently rolls for Stun and Blind
5. Stun and Blind durations: standard status effect durations (Stun 5s, Blind 30s), subject to target's resistance formula
6. Card modifiers apply (% cards and +ATK cards work)
7. Pre-renewal: 3x3 AoE = 150 UE units radius (1.5 cells from center)

**Implementation Notes:**
- Check `hasBuff(player, 'hiding')` before execution -- if not hidden, error
- Remove hiding buff and broadcast `skill:buff_removed`
- Find all enemies in `AoE_RADIUS = 150` UE units from caster position
- Apply damage to each, roll stun/blind per target
- Use `applyStatusEffect()` for stun/blind with boss immunity checks

---

### 4.5 Intimidate / Snatch (ID 1704) -- Active

**rAthena ID:** 219 (RG_INTIMIDATE)

| Property | Value |
|----------|-------|
| Type | Active, Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Back Stab (1701) Lv4, Raid (1703) Lv5 |
| Boss Restriction | Cannot target Boss/MVP monsters |

**SP Cost and Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 13 | 16 | 19 | 22 | 25 |
| ATK% | 130 | 160 | 190 | 220 | 250 |

**Formula:** `ATK% = 100 + level * 30`, `SP = 10 + level * 3`

**Teleport Mechanic:**
1. Deal physical damage to the target
2. If target survives and is NOT a Boss:
   - Teleport BOTH caster and target to a random walkable position on the same map
   - Update both positions in the server cache and broadcast
3. If target is a Boss: deal damage only, NO teleport (with warning message)
4. If used in restricted maps (GvG, Battleground, etc.): damage only, NO teleport

**PvE vs PvP:**
- PvE: Teleports both to random position on the map (useful for fleeing or repositioning)
- PvP: Teleports both (used to isolate enemies in WoE/PvP)

**Implementation Notes:**
- Boss check: `if (enemy.modeFlags?.boss)` -> damage only, skip teleport
- Random position generation: pick random x/y within zone bounds
- Broadcast `player:teleport` for caster, `enemy:teleport` for the monster
- Update `player.x/y/z` and `enemy.x/y/z` on server

---

### 4.6 Sword Mastery -- Rogue (ID 1705) -- Passive

**rAthena ID:** 2 (SM_SWORD) -- shared skill

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Effect | +4 ATK per level with Daggers and 1H Swords |
| Prerequisites | None |

This is the same skill as Swordsman Sword Mastery (ID 100), but available in the Rogue skill tree. The passive bonus stacks additively -- at Lv10, +40 flat ATK with Daggers/1H Swords. This flat ATK bypasses all multipliers (added as MasteryATK after size/element/card modifiers).

**Implementation:** Use the existing `getPassiveSkillBonuses()` pattern. Check for `learnedSkills[1705]` in addition to `learnedSkills[100]` (do NOT stack both -- use the higher level if somehow both are learned).

---

### 4.7 Vulture's Eye -- Rogue (ID 1706) -- Passive

**rAthena ID:** 44 (AC_VULTURE) -- shared skill

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Effect | +1 bow attack range per level, +1 HIT per level |
| Prerequisites | None |

Same as Archer's Vulture's Eye (ID 301). At Lv10: +10 bow range cells, +10 HIT.

**Implementation:** Check for `learnedSkills[1706]` in `getPassiveSkillBonuses()` alongside `learnedSkills[301]`. Do NOT stack both.

---

### 4.8 Double Strafe -- Rogue (ID 1707) -- Active

**rAthena ID:** 46 (AC_DOUBLE) -- shared skill

| Property | Value |
|----------|-------|
| Type | Active, Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells (~450 UE units, extended by Vulture's Eye) |
| Element | Weapon element (arrow element in RO) |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0.1 seconds |
| Cooldown | 0 |
| Prerequisites | Vulture's Eye (1706) Lv10 |
| Weapon | Bow ONLY, consumes 1 arrow |
| Hits | 2 |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% per hit | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 |
| Total ATK% | 200 | 220 | 240 | 260 | 280 | 300 | 320 | 340 | 360 | 380 |

Same handler as Archer's Double Strafe (ID 303). The Rogue version simply has a different prerequisite (Vulture's Eye Rogue Lv10 instead of Archer's Vulture's Eye Lv10).

---

### 4.9 Remove Trap -- Rogue (ID 1708) -- Active

**rAthena ID:** 124 (HT_REMOVETRAP) -- shared skill

| Property | Value |
|----------|-------|
| Type | Active, Utility |
| Max Level | 1 |
| Target | Ground (trap) |
| Range | 2 cells (~100 UE units) |
| SP Cost | 5 |
| Prerequisites | Double Strafe (1707) Lv5 |

Removes a Hunter/Trapper's ground trap and returns the trap item. Primarily a PvP/WoE utility skill.

**Implementation:** Deferred -- requires trap system to be implemented first (Hunter skills).

---

### 4.10 Steal Coin / Mug (ID 1709) -- Active

**rAthena ID:** 211 (RG_STEALCOIN)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy (monsters only) |
| Range | Melee (1 cell) |
| SP Cost | 15 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Snatcher (1700) Lv4 |
| Restriction | Cannot target Boss monsters or players |
| One-time | Can only steal Zeny once per monster |

**Base Success Rate Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Base % | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |

**Success Rate Formula (rAthena/iRO Wiki):**
```
SuccessRate = (DEX / 2 + LUK / 2 + 2 * (casterLevel - targetLevel) + baseRate * 10) / 10
```
Where `baseRate = skillLevel` (1-10%).

**Zeny Stolen Formula:**
```
Zeny = random(0, 2 * monsterLevel + 1) + 8 * monsterLevel
```
Example: Monster Level 50 -> `random(0, 101) + 400` -> 400-501 Zeny.

**Mechanic:**
1. Check target is not Boss (`enemy.modeFlags?.boss`)
2. Check not already stolen from (`enemy.zenyStolen`)
3. Roll success rate
4. On success: award Zeny to player, mark monster as `enemy.zenyStolen = true`
5. Drawing aggro: successful cast draws monster aggression (`setEnemyAggro`)

**Implementation Notes:**
- Add `zenyStolen` flag to enemy tracking (separate from `stolenBy` for item steal)
- On success: `player.zeny += stolenZeny`, emit `zeny:update`
- On failure: emit `skill:effect_damage` with `damage: 0, isMiss: true`

---

### 4.11 Divest Helm / Strip Helm (ID 1710) -- Active

**rAthena ID:** 218 (RG_STRIPHELM)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Cast Time | 1.0 seconds (pre-renewal) |
| After-Cast Delay | 1.0 seconds |
| Cooldown | 0 |
| Prerequisites | Steal Coin (1709) Lv2 |
| Interruptible | Yes (cast can be interrupted by damage) |

**SP Cost Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 |

**Success Rate Formula (rAthena Official):**
```
rate = 50 * (skillLevel + 1) + 2 * (casterDEX - targetDEX)
```
This is per 1000 (divide by 10 for percentage). So at Lv5 with equal DEX: `50 * 6 / 10 = 30%`.

Simplified per 100: `rate% = 5 * (skillLevel + 1) + 0.2 * (casterDEX - targetDEX)`

| Level | Base % (equal DEX) |
|-------|-------------------|
| 1 | 10% |
| 2 | 15% |
| 3 | 20% |
| 4 | 25% |
| 5 | 30% |

**Duration Formula:**
```
duration(ms) = 60000 + 15000 * skillLevel + 500 * (casterDEX - targetDEX)
```

| Level | Base Duration (equal DEX) |
|-------|--------------------------|
| 1 | 75s (1:15) |
| 2 | 90s (1:30) |
| 3 | 105s (1:45) |
| 4 | 120s (2:00) |
| 5 | 135s (2:15) |

**Effect on Players (PvP):** Removes equipped headgear, prevents re-equipping any headgear for duration.

**Effect on Monsters:** Temporarily reduces INT by 40% (does not affect SP).

**Blocked by:** Chemical Protection (Alchemist skill)

---

### 4.12 Divest Shield / Strip Shield (ID 1711) -- Active

**rAthena ID:** 216 (RG_STRIPSHIELD)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Cast Time | 1.0 seconds |
| After-Cast Delay | 1.0 seconds |
| Cooldown | 0 |
| Prerequisites | Divest Helm (1710) Lv5 |

**SP Cost Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 12 | 14 | 16 | 18 | 20 |

**Success Rate Formula:** Same as Divest Helm (see section 4.11).

**Duration Formula:** Same as Divest Helm (see section 4.11).

**Effect on Players (PvP):** Removes equipped shield, prevents re-equipping shields for duration.

**Effect on Monsters:** Temporarily reduces hard DEF by 15%.

**Blocked by:** Chemical Protection

---

### 4.13 Divest Armor / Strip Armor (ID 1712) -- Active

**rAthena ID:** 217 (RG_STRIPARMOR)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Cast Time | 1.0 seconds |
| After-Cast Delay | 1.0 seconds |
| Cooldown | 0 |
| Prerequisites | Divest Shield (1711) Lv5 |

**SP Cost Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 17 | 19 | 21 | 23 | 25 |

**Success Rate Formula:** Same as Divest Helm (see section 4.11).

**Duration Formula:** Same as Divest Helm (see section 4.11).

**Effect on Players (PvP):** Removes equipped armor, prevents re-equipping armor for duration. Strips armor element card (target becomes Neutral element until duration ends).

**Effect on Monsters:** Temporarily reduces VIT by 40% (does not reduce current HP, affects soft DEF).

**Blocked by:** Chemical Protection

---

### 4.14 Divest Weapon / Strip Weapon (ID 1713) -- Active

**rAthena ID:** 215 (RG_STRIPWEAPON)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Cast Time | 1.0 seconds |
| After-Cast Delay | 1.0 seconds |
| Cooldown | 0 |
| Prerequisites | Divest Armor (1712) Lv5 |

**SP Cost Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP | 17 | 19 | 21 | 23 | 25 |

**Success Rate Formula:** Same as Divest Helm (see section 4.11).

**Duration Formula:** Same as Divest Helm (see section 4.11).

**Effect on Players (PvP):** Removes equipped weapon, prevents re-equipping weapons for duration. Target can only punch (bare fist ATK).

**Effect on Monsters:** Temporarily reduces ATK by 25%.

**Blocked by:** Chemical Protection

---

### 4.15 Plagiarism (ID 1714) -- Passive

**rAthena ID:** 225 (RG_PLAGIARISM)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Intimidate (1704) Lv5 |
| Effect | Copy the last skill that damaged the Rogue |
| Copy limit | 1 skill at a time (new copy overwrites old) |
| Level cap | Copied skill level capped at Plagiarism skill level |
| ASPD bonus | +1% per skill level |

**Copy Rules:**

**CAN be copied (skills from 1st and 2nd classes that deal damage to the Rogue):**

| Source Class | Copyable Skills |
|-------------|-----------------|
| Swordsman | Bash, Magnum Break |
| Knight | Bowling Bash, Charge Attack |
| Crusader | Grand Cross, Holy Cross, Shield Boomerang, Shield Charge (Smite) |
| Mage | Cold Bolt, Fire Ball, Fire Bolt, Fire Wall, Frost Diver, Lightning Bolt, Napalm Beat, Soul Strike, Thunderstorm |
| Wizard | Earth Spike, Fire Pillar, Frost Nova, Heaven's Drive, Jupitel Thunder, Lord of Vermilion, Meteor Storm, Sight Rasher, Storm Gust, Water Ball, Sight Blaster |
| Sage | Earth Spike, Heaven's Drive |
| Archer | Arrow Repel, Arrow Shower, Double Strafe |
| Hunter | Blast Mine, Claymore Trap, Land Mine, Phantasmic Arrow, Freezing Trap |
| Merchant | Mammonite |
| Alchemist | Acid Terror, Bomb (Demonstration) |
| Thief | Envenom, Sand Attack, Stone Fling (Throw Stone) |
| Assassin | Venom Splasher, Venom Knife |
| Rogue | Back Stab, Double Strafe, Sightless Mind (Raid) |
| Acolyte | Holy Light, Ruwach (while hiding) |
| Priest | Aspersio, B.S. Sacramenti, Magnus Exorcismus, Turn Undead |
| Monk | Raging Trifecta Blow (Combo), Excruciating Palm (Combo) |

**CANNOT be copied:**
- All monster-exclusive skills
- All Transcendent class skills (Lord Knight, High Wizard, etc.)
- Non-damaging skills (buffs, heals, etc.) -- except Aspersio
- Skills with weapon class requirements the Rogue cannot meet (Sonic Blow needs Katar, but Rogue can't equip Katar)
- Brandish Spear, Pierce, Spear Boomerang, Spear Stab (need spear)
- Shield Reflect (non-damaging)
- Cart Revolution (need cart)
- Grimtooth, Sonic Blow (need katar)
- Guillotine Fist, Occult Impaction, Raging Quadruple Blow, Raging Thrust, Throw Spirit Sphere (Monk skills)
- Blitz Beat (falcon mechanic)
- Melody Strike, Unchained Serenade, Slinging Arrow (Bard/Dancer)
- Resurrection, Sanctuary (non-damaging)
- All Gunslinger skills
- All Ninja throwing skills (Throw Shuriken/Kunai/Huuma)

**Weapon Requirement Caveat:** If the Rogue copies a skill that requires a weapon type they CAN equip (e.g., Bash with sword, Bowling Bash), they can use it. If the skill requires a weapon type they CANNOT equip (e.g., spear skills), the skill is technically "copied" but unusable.

**ASPD Bonus:** Plagiarism grants a passive ASPD bonus of +1% per level (Lv10 = +10% ASPD). This is separate from the skill copying mechanic.

**Pre-Amatsu Note:** In some pre-renewal configurations, the copied skill is lost when changing maps. In modern pre-renewal servers, the copied skill persists across map changes. For Sabri_MMO, the copied skill should persist (stored in DB).

---

### 4.16 Gangster's Paradise (ID 1715) -- Passive

**rAthena ID:** 223 (RG_GANGSTER)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 1 |
| AoE | 3x3 cells |
| Prerequisites | Raid (1703) Lv1 |

**Mechanic:**
- When 2 or more Rogues/Stalkers are sitting within 3x3 cells of each other, nearby non-Boss monsters will not attack any player in the group
- Breaks when any player in the group stands up or moves
- Boss monsters are NOT affected (they will still attack)
- The sitting check must be continuous -- if one Rogue stands, all lose protection

**Implementation:** LOW priority. Requires sitting state tracking and proximity checks in the AI tick. Deferred to future phase.

---

### 4.17 Compulsion Discount / Haggle (ID 1716) -- Passive

**rAthena ID:** 224 (RG_COMPULSION)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Gangster's Paradise (1715) Lv1 |

**Discount Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Discount % | 9 | 13 | 17 | 21 | 25 |

**Formula:** `discount% = 5 + level * 4`

At max level, this is 1% better than the Merchant's Discount skill (24% at Lv10).

**Implementation:** In the NPC shop buy handler, check if player has Compulsion Discount. Apply price reduction: `price = floor(originalPrice * (100 - discount) / 100)`. Does not stack with Merchant's Discount (use the higher value).

---

### 4.18 Scribble / Graffiti (ID 1717) -- Quest Skill

**rAthena ID:** 220 (RG_GRAFFITI)

| Property | Value |
|----------|-------|
| Type | Active, Utility |
| Max Level | 1 |
| Target | Ground |
| Range | 1 cell |
| SP Cost | 15 |
| Duration | 180 seconds |
| Catalyst | 1 Red Gemstone consumed per cast |
| Prerequisites | Quest skill (learned from NPC quest) |

**Effect:** Places a text graffiti on the ground (purely cosmetic). Players can write a custom message that appears on the ground tile.

**Implementation:** LOWEST priority. Purely cosmetic, no gameplay impact. Deferred indefinitely.

---

### 4.19 Close Confine (ID 1718) -- Quest Skill

**rAthena ID:** 1005 (RG_CLOSECONFINE)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 1 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| SP Cost | 25 (pre-renewal) |
| Cast Time | 0 (instant) |
| Duration | 10 seconds (pre-renewal) |
| FLEE Bonus | +10 FLEE for caster (pre-renewal) |
| Prerequisites | Quest skill (Rogue Platinum Skills Quest) |
| Boss Restriction | Cannot target Boss monsters |

**Mechanic:**
1. Lock BOTH caster and target in place (neither can move)
2. Both can still attack, use skills, and use items
3. Caster gains +10 FLEE during the lock
4. Duration: 10 seconds

**Break Conditions:**
- Either caster or target dies
- Either party teleports (Fly Wing, Butterfly Wing, Teleport skill)
- Target enters Hiding status
- Knockback occurs to either party
- Duration expires

**Implementation Notes:**
- Apply a `close_confine` buff to BOTH caster and target
- Buff includes `isConfiner: true/false` flag to distinguish who initiated
- In `player:position` handler: reject movement for both parties
- Caster gets `bonusFLEE += 10` via buff modifiers
- On any break condition: remove buff from both, broadcast `skill:buff_removed`
- Check `enemy.modeFlags?.boss` before applying

---

## 5. Plagiarism System Architecture

This is the most complex new system required for the Rogue class.

### 5.1 Data Storage

**Server-side (in-memory per player):**
```js
player.plagiarizedSkill = {
    skillId: 200,           // Original skill ID (e.g., Cold Bolt = 200)
    skillName: 'cold_bolt', // Skill internal name
    displayName: 'Cold Bolt',
    maxLevel: 10,           // The original skill's max level
    copiedLevel: 5,         // The level it was used at when copied
    usableLevel: 5,         // min(copiedLevel, plagiarismLevel)
    copiedAt: Date.now(),   // When it was copied
    sourceType: 'player',   // 'player' or 'enemy' (who cast it on us)
    sourceId: 12345         // Character/enemy ID of the caster
};
```

**Database (persistent across sessions):**
```sql
ALTER TABLE characters ADD COLUMN plagiarized_skill_id INTEGER DEFAULT NULL;
ALTER TABLE characters ADD COLUMN plagiarized_skill_level INTEGER DEFAULT 0;
```

### 5.2 Copy Trigger

When any skill damages the Rogue via `skill:effect_damage`:
1. Check if Rogue has Plagiarism learned (learnedSkills[1714] > 0)
2. Check if the skill is in the COPYABLE_SKILLS whitelist
3. Check if the Rogue does NOT have the Preserve buff (Stalker skill, future)
4. Copy the skill: `player.plagiarizedSkill = { ... }`
5. Cap the usable level: `usableLevel = min(copiedLevel, plagiarismLevel)`
6. Emit `plagiarism:skill_copied` to the Rogue: `{ skillId, displayName, usableLevel }`

### 5.3 Using the Copied Skill

When `skill:use` is received:
1. Check if `skillId` matches `player.plagiarizedSkill.skillId`
2. If so, execute the original skill's handler with `usableLevel` as the level
3. SP cost is determined by the skill at `usableLevel`
4. Cast time, cooldown, ACD all follow the original skill at `usableLevel`
5. The plagiarized skill appears in the Rogue's hotbar as a special "copied" skill

### 5.4 Overwrite Prevention (Future: Preserve)

The Stalker skill `Preserve` (ID: 475) prevents Plagiarism from overwriting the currently copied skill. This is NOT needed for the Rogue phase -- only for the Stalker (transcendent) phase.

### 5.5 Weapon Requirement Validation

When using a copied skill, verify weapon requirements:
- Copied Bowling Bash: needs 1H/2H Sword -> Rogue can use (has Sword Mastery)
- Copied Sonic Blow: needs Katar -> Rogue CANNOT equip Katar -> `skill:error`
- Copied Double Strafe: needs Bow -> Rogue CAN equip Bow -> OK

**Validation:** Look up the original skill definition, check if the skill has weapon type requirements, verify the Rogue's equipped weapon type matches.

### 5.6 Copyable Skills Whitelist

```js
const PLAGIARISM_COPYABLE_SKILLS = new Set([
    // Swordsman
    'bash', 'magnum_break',
    // Knight
    'bowling_bash', 'charge_attack',
    // Crusader
    'grand_cross', 'holy_cross', 'shield_boomerang', 'shield_charge',
    // Mage
    'cold_bolt', 'fire_ball', 'fire_bolt', 'fire_wall', 'frost_diver',
    'lightning_bolt', 'napalm_beat', 'soul_strike', 'thunderstorm',
    // Wizard
    'earth_spike', 'fire_pillar', 'frost_nova', 'heavens_drive',
    'jupitel_thunder', 'lord_of_vermilion', 'meteor_storm',
    'sight_rasher', 'storm_gust', 'water_ball', 'sight_blaster',
    // Sage
    'earth_spike_sage', 'heavens_drive_sage',
    // Archer
    'arrow_repel', 'arrow_shower', 'double_strafe',
    // Hunter
    'blast_mine', 'claymore_trap', 'land_mine', 'phantasmic_arrow', 'freezing_trap',
    // Merchant
    'mammonite',
    // Alchemist
    'acid_terror', 'bomb',
    // Thief
    'envenom', 'sand_attack', 'throw_stone',
    // Assassin
    'venom_splasher', 'venom_knife',
    // Rogue
    'back_stab', 'double_strafe_rogue', 'raid',
    // Acolyte
    'holy_light', 'ruwach',
    // Priest
    'aspersio', 'bs_sacramenti', 'magnus_exorcismus', 'turn_undead',
    // Monk
    'raging_trifecta_blow', 'excruciating_palm',
]);
```

### 5.7 Socket.io Events for Plagiarism

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `plagiarism:skill_copied` | Server -> Rogue | `{ skillId, displayName, usableLevel, icon }` | Skill was copied |
| `plagiarism:skill_lost` | Server -> Rogue | `{ reason }` | Copied skill lost (overwritten/cleared) |
| `plagiarism:data` | Server -> Rogue | `{ skillId, displayName, usableLevel, icon }` or `null` | Full state on login |

### 5.8 Client-Side UI

- The copied skill appears as a special entry in the skill tree (separate row/section)
- Can be dragged to hotbar like any other skill
- Hotbar icon shows the original skill's icon with a special "Plagiarism" border/overlay
- Tooltip shows: "[Copied] Cold Bolt Lv5 (via Plagiarism)"

---

## 6. Strip Equipment System Architecture

### 6.1 Strip Buff Structure

When a strip skill succeeds, apply a status debuff to the target:

```js
{
    name: 'strip_weapon',      // or 'strip_shield', 'strip_armor', 'strip_helm'
    skillId: 1713,             // The strip skill that applied this
    casterId: characterId,     // Who stripped the target
    duration: calculatedDuration,
    appliedAt: now,
    expiresAt: now + calculatedDuration,
    effects: {
        stripType: 'weapon',   // Which equipment slot is stripped
        // Monster effects:
        atkReduction: 0.25,    // 25% ATK reduction (weapon)
        defReduction: 0.15,    // 15% hard DEF reduction (shield)
        vitReduction: 0.40,    // 40% VIT reduction (armor)
        intReduction: 0.40     // 40% INT reduction (helm)
    }
}
```

### 6.2 Monster Effect Application

When a monster is stripped:
- **Strip Weapon:** `monster.buffATKMultiplier = 0.75` (25% ATK reduction)
- **Strip Shield:** `monster.buffHardDefReduction = 0.15` (15% hard DEF reduction)
- **Strip Armor:** `monster.buffVITMultiplier = 0.60` (40% VIT reduction, reduces soft DEF)
- **Strip Helm:** `monster.buffINTMultiplier = 0.60` (40% INT reduction, reduces soft MDEF)

These modifiers are applied in the damage pipeline via `getBuffStatModifiers()`.

### 6.3 Player Effect Application (PvP, future)

When a player is stripped:
- The specific equipment slot is forcibly unequipped (server-side)
- A `strip_*` debuff prevents re-equipping that slot for the duration
- Equipment bonuses from the stripped item are removed for the duration
- On debuff expiry: the slot is unlocked (player can manually re-equip)
- Emit `equipment:stripped` to the target with slot info

### 6.4 Success Rate Calculation

```js
function calculateStripChance(casterDEX, targetDEX, skillLevel) {
    // rAthena official formula (per 1000)
    const rate = 50 * (skillLevel + 1) + 2 * (casterDEX - targetDEX);
    // Convert to percentage (divide by 10)
    return Math.max(0, rate / 10);
}

function calculateStripDuration(casterDEX, targetDEX, skillLevel) {
    // rAthena official formula
    return Math.max(5000, 60000 + 15000 * skillLevel + 500 * (casterDEX - targetDEX));
}
```

### 6.5 Chemical Protection Block

If the target has Chemical Protection (Alchemist buff, future):
- All strip skills automatically fail
- Emit `skill:error` with "Target is protected by Chemical Protection"

### 6.6 Socket.io Events for Strip

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `strip:applied` | Server -> Zone | `{ targetId, isEnemy, stripType, duration }` | Strip debuff applied |
| `strip:removed` | Server -> Zone | `{ targetId, isEnemy, stripType, reason }` | Strip debuff expired/removed |

---

## 7. New Systems Required

### 7.1 Plagiarism Copy System (COMPLEX -- HIGH Priority)

**Files affected:** `index.js` (skill damage handlers), `ro_skill_data_2nd.js` (skill def)

**Requirements:**
1. `PLAGIARISM_COPYABLE_SKILLS` whitelist (Set of skill names)
2. `player.plagiarizedSkill` in-memory storage
3. DB columns for persistence (`plagiarized_skill_id`, `plagiarized_skill_level`)
4. Copy trigger on any `skill:effect_damage` that hits a Rogue
5. `skill:use` handler must check `plagiarizedSkill` for unrecognized skill IDs
6. Weapon requirement validation for copied skills
7. Socket events: `plagiarism:skill_copied`, `plagiarism:skill_lost`, `plagiarism:data`
8. Hotbar integration: copied skill assignable to hotbar slots

**Estimated effort:** 4-6 hours (most complex Rogue system)

### 7.2 Strip Equipment Debuff System (MEDIUM Priority)

**Files affected:** `index.js` (strip handlers), `ro_buff_system.js` (buff type)

**Requirements:**
1. 4 strip buff types: `strip_weapon`, `strip_shield`, `strip_armor`, `strip_helm`
2. Success rate formula: `5 * (skillLevel + 1) + 0.2 * (casterDEX - targetDEX)` %
3. Duration formula: `60000 + 15000 * skillLevel + 500 * (casterDEX - targetDEX)` ms
4. Monster stat modifiers (ATK -25%, hard DEF -15%, VIT -40%, INT -40%)
5. Buff modifier integration in `getBuffStatModifiers()`
6. Chemical Protection immunity check (future)
7. Cast time: 1 second (with DEX reduction as per cast time formula)
8. Interruptible cast

**Estimated effort:** 3-4 hours

### 7.3 Backstab Positioning System (MEDIUM Priority)

**Files affected:** `index.js` (Back Stab handler)

**Requirements:**
1. Always-hit damage (forceHit: true)
2. Dagger double-hit (2 hits at full ATK%)
3. Bow half-damage (0.5x modifier)
4. Teleport caster behind target (update server position)
5. Target facing reversal (cosmetic, emit facing data)
6. Cancel Hiding on use (if hidden)

**Estimated effort:** 2-3 hours

### 7.4 Auto-Steal on Attack (Snatcher) (LOW Priority)

**Files affected:** `index.js` (combat tick auto-attack)

**Requirements:**
1. Check Rogue has Snatcher passive on each melee auto-attack
2. Roll trigger chance (7-20%)
3. On trigger: execute Steal's item acquisition logic
4. Shares `enemy.stolenBy` tracker with Steal skill
5. Only on melee (not bow/ranged attacks)

**Estimated effort:** 1-2 hours (depends on Steal item system from Thief audit)

### 7.5 Zeny Theft System (Steal Coin) (LOW Priority)

**Files affected:** `index.js` (Steal Coin handler)

**Requirements:**
1. Success rate formula with DEX/LUK/level diff
2. Zeny calculation: `random(0, 2*monLevel+1) + 8*monLevel`
3. One-time per monster (`enemy.zenyStolen` flag)
4. Boss immunity check
5. Award Zeny to player, emit `zeny:update`
6. Aggro drawing on success

**Estimated effort:** 1-2 hours

### 7.6 Hiding Movement (Tunnel Drive) (LOW Priority)

**Files affected:** `index.js` (player:position handler)

**Requirements:**
1. Check if player has Tunnel Drive passive when in Hiding
2. If yes: allow movement at reduced speed (26-50% of normal)
3. Speed enforcement: cap max distance per position update
4. Works with existing Hiding SP drain tick

**Estimated effort:** 1 hour

### 7.7 Close Confine Movement Lock (LOW Priority)

**Files affected:** `index.js` (Close Confine handler)

**Requirements:**
1. Lock both caster and target in place
2. Apply `close_confine` buff to both with `isConfiner` flag
3. Reject position updates for both while active
4. Caster +10 FLEE via buff modifiers
5. Break conditions: death, teleport, hiding, knockback
6. Boss immunity

**Estimated effort:** 2 hours

### 7.8 Raid AoE from Hiding (LOW Priority)

**Files affected:** `index.js` (Raid handler)

**Requirements:**
1. Verify Hiding status before use
2. Cancel Hiding
3. Deal AoE damage in 3x3 around caster
4. Roll Stun/Blind per target hit
5. Standard physical damage pipeline

**Estimated effort:** 1-2 hours

---

## 8. Existing Code Audit

### 8.1 Skill Definitions (`ro_skill_data_2nd.js`)

All 19 Rogue skills are already defined (IDs 1700-1718). Audit of current definitions vs canonical values:

| ID | Skill | Issue | Correct Value |
|----|-------|-------|---------------|
| 1700 | Snatcher | effectValue formula uses `70+floor((i+1)*15)` | Should be fixed table: [70,80,100,110,130,140,160,170,190,200] (per 1000) |
| 1701 | Back Stab | cooldown: 500 | Should be 0 (ASPD-based delay, not cooldown) |
| 1701 | Back Stab | afterCastDelay: 0 | Should be 500 (0.5s global delay) |
| 1701 | Back Stab | prerequisites: [] | Should be `[{ skillId: 1709, level: 4 }]` (Steal Coin Lv4) |
| 1703 | Raid | AoE described as "7x7" in description | Pre-renewal is 3x3 (fix description) |
| 1703 | Raid | cooldown: 1000 | Should be 0 |
| 1704 | Intimidate | SP formula: 13+i*2 gives 13,15,17,19,21 | Canonical: 13,16,19,22,25 (i.e., 10+i*3) |
| 1709 | Steal Coin | cooldown: 500 | Should be 0 |
| 1710 | Divest Helm | castTime: 0 | Should be 1000 (1.0s cast) |
| 1710 | Divest Helm | SP: 17 all levels | Should be 12,14,16,18,20 |
| 1710 | Divest Helm | effectValue: 10+i*5 gives 10-30% | Canonical: `5*(i+2)` = 10,15,20,25,30% |
| 1710 | Divest Helm | duration: 75000+i*15000 | Canonical: `60000+15000*(i+1)` = 75,90,105,120,135s (CORRECT) |
| 1710 | Divest Helm | afterCastDelay: 0 | Should be 1000 (1.0s ACD) |
| 1710 | Divest Helm | prerequisites: [] | Should be `[{ skillId: 1709, level: 2 }]` |
| 1711 | Divest Shield | castTime: 0 | Should be 1000 |
| 1711 | Divest Shield | SP: 17 all levels | Should be 12,14,16,18,20 |
| 1711 | Divest Shield | afterCastDelay: 0 | Should be 1000 |
| 1712 | Divest Armor | castTime: 0 | Should be 1000 |
| 1712 | Divest Armor | afterCastDelay: 0 | Should be 1000 |
| 1713 | Divest Weapon | castTime: 0 | Should be 1000 |
| 1713 | Divest Weapon | afterCastDelay: 0 | Should be 1000 |
| 1715 | Gangster's Paradise | prerequisites: `[{ skillId: 1703, level: 1 }]` | Canonical: Strip Shield Lv3 -- BUT some sources say Raid Lv1. rAthena uses `RG_STRIPSHIELD` Lv3. Use: `[{ skillId: 1711, level: 3 }]` |
| 1718 | Close Confine | SP: 25 | Pre-renewal: 25 (CORRECT for pre-renewal) |
| 1718 | Close Confine | duration: 15000 | Pre-renewal: 10000 (10 seconds, not 15) |
| 1718 | Close Confine | effectValue: 10 | FLEE bonus: +10 (pre-renewal) -- CORRECT |

### 8.2 Skill Handlers in `index.js`

**Status: NO Rogue handlers exist.** All 19 skills need handlers added to the `skill:use` switch block (except passives: Snatcher, Tunnel Drive, Sword Mastery Rogue, Vulture's Eye Rogue, Plagiarism, Gangster's Paradise, Compulsion Discount = 7 passives without handlers).

Active skills needing handlers (12 total):
1. Back Stab (1701)
2. Raid (1703)
3. Intimidate (1704)
4. Double Strafe Rogue (1707) -- can reuse Archer's handler
5. Remove Trap Rogue (1708) -- deferred (trap system)
6. Steal Coin (1709)
7. Divest Helm (1710)
8. Divest Shield (1711)
9. Divest Armor (1712)
10. Divest Weapon (1713)
11. Scribble (1717) -- deferred (cosmetic)
12. Close Confine (1718)

### 8.3 Passive Skill Bonuses in `getPassiveSkillBonuses()`

**Rogue passives needing integration (7 total):**
1. **Snatcher (1700):** `autoStealChance` field (new)
2. **Tunnel Drive (1702):** `hiddenMoveSpeed` field (new)
3. **Sword Mastery Rogue (1705):** `bonusATK` with dagger/1H sword weapon check (reuse Swordsman pattern, don't stack with ID 100)
4. **Vulture's Eye Rogue (1706):** `bonusHIT`, `bonusRange` (reuse Archer pattern, don't stack with ID 301)
5. **Plagiarism (1714):** `plagiarismLevel` field, `aspdBonus` (+1% per level)
6. **Gangster's Paradise (1715):** `gangsterParadise: true` flag (new, AI integration)
7. **Compulsion Discount (1716):** `compulsionDiscount` field (new, shop integration)

---

## 9. Implementation Priority

### Phase 1: Skill Definition Fixes (30 min)

Fix all issues identified in section 8.1 in `ro_skill_data_2nd.js`:
- Backstab: fix prerequisites, cooldown/ACD swap
- Intimidate: fix SP formula
- All Divest skills: add cast time 1000, ACD 1000, fix SP costs
- Close Confine: fix duration to 10000
- Divest Helm: fix prerequisites
- Gangster's Paradise: fix prerequisites
- Raid: fix description (3x3 not 7x7)

### Phase 2: Passive Skill Integration (1-2 hours)

Add all 7 Rogue passives to `getPassiveSkillBonuses()`:
1. Sword Mastery Rogue (bonusATK with weapon check)
2. Vulture's Eye Rogue (bonusHIT, bonusRange)
3. Snatcher (autoStealChance)
4. Tunnel Drive (hiddenMoveSpeed)
5. Plagiarism (plagiarismLevel, aspdBonus)
6. Gangster's Paradise (flag)
7. Compulsion Discount (discount%)

### Phase 3: Core Combat Skills (3-4 hours)

Implement handlers for the primary damage skills:
1. **Back Stab** -- always-hit, dagger double-hit, bow half-damage, position teleport
2. **Raid** -- AoE from Hiding, stun/blind chance
3. **Intimidate** -- damage + teleport both, boss immunity
4. **Double Strafe Rogue** -- reuse Archer handler routing

### Phase 4: Strip Equipment System (3-4 hours)

Implement the 4 Divest skills:
1. Strip success rate formula
2. Strip duration formula
3. Strip debuff application (4 buff types)
4. Monster stat modifiers in `getBuffStatModifiers()`
5. Buff expiration in buff tick
6. Cast time with interruption

### Phase 5: Economic Skills (1-2 hours)

1. **Steal Coin** -- Zeny theft, success formula, boss immunity
2. **Compulsion Discount** integration in shop handler

### Phase 6: Plagiarism System (4-6 hours)

The most complex system -- implement last:
1. Copyable skills whitelist
2. Copy trigger on skill damage
3. In-memory + DB storage
4. `skill:use` handler integration for copied skill
5. Weapon requirement validation
6. Socket events
7. Client integration (hotbar, skill tree)

### Phase 7: Utility Skills (2-3 hours)

1. **Close Confine** -- movement lock, FLEE bonus, break conditions
2. **Snatcher** auto-steal integration in combat tick
3. **Tunnel Drive** movement while hidden
4. **Remove Trap** -- deferred until trap system exists

### Phase 8: Polish & Quest Skills (1 hour)

1. Scribble -- cosmetic, lowest priority
2. VFX configs for all skills
3. Testing and edge cases

**Total estimated effort: 16-24 hours across all phases**

---

## 10. Integration Points

### 10.1 Combat Tick (Auto-Attack)

- **Snatcher:** After each melee auto-attack hit, check for Snatcher passive -> roll auto-steal
- **Plagiarism ASPD:** Apply +1-10% ASPD bonus from Plagiarism passive in `calculateASPD()`

### 10.2 Skill Damage Handlers

- **Plagiarism copy trigger:** In ALL `skill:effect_damage` broadcasts to a Rogue target, check if the skill is copyable and copy it
- **Back Stab:** Custom handler with forceHit, weapon modifiers, position teleport
- **Raid:** Hiding check, AoE damage, stun/blind application
- **Intimidate:** Damage + random teleport both (boss immunity check)

### 10.3 Buff System (`ro_buff_system.js`)

New buff types needed:
- `strip_weapon` -- ATK -25% on monsters
- `strip_shield` -- hard DEF -15% on monsters
- `strip_armor` -- VIT -40% on monsters
- `strip_helm` -- INT -40% on monsters
- `close_confine_caster` -- movement lock + FLEE +10
- `close_confine_target` -- movement lock
- `raid_debuff` -- +30% damage taken (20% for bosses) -- NOTE: this is a Renewal mechanic, NOT pre-renewal

### 10.4 Damage Formula Integration

In `getBuffStatModifiers()`, add checks for strip debuffs:
```js
if (hasBuff(entity, 'strip_weapon')) {
    mods.atkMultiplier *= 0.75;  // -25% ATK
}
if (hasBuff(entity, 'strip_shield')) {
    mods.hardDefReduction = 0.15; // -15% hard DEF
}
if (hasBuff(entity, 'strip_armor')) {
    mods.vitMultiplier *= 0.60;  // -40% VIT
}
if (hasBuff(entity, 'strip_helm')) {
    mods.intMultiplier *= 0.60;  // -40% INT
}
```

### 10.5 NPC Shop Handler

Check Compulsion Discount in shop buy logic:
```js
const discountLv = player.learnedSkills?.[1716] || 0;
if (discountLv > 0) {
    const discount = 5 + discountLv * 4; // 9-25%
    price = Math.floor(price * (100 - discount) / 100);
}
```

### 10.6 Enemy AI System

- **Gangster's Paradise:** In `findAggroTarget()`, skip players that are sitting with 2+ Rogues nearby (requires sitting state + proximity check)
- **Close Confine:** Target under close_confine cannot be knocked back

### 10.7 Player Position Handler

- **Tunnel Drive:** If player is in Hiding AND has Tunnel Drive, allow movement at reduced speed
- **Close Confine:** Reject position updates for both confiner and confined target

### 10.8 Hiding System (from Thief)

- **Tunnel Drive:** Modifies existing Hiding movement block
- **Raid:** Requires Hiding, cancels Hiding on use
- **Back Stab:** Can be used from Hiding (pre-renewal), cancels Hiding

---

## 11. Rogue-Specific Constants and Data Tables

### 11.1 Skill ID Ranges

```js
// Rogue skill IDs
const ROGUE_SKILL_IDS = {
    SNATCHER: 1700,
    BACK_STAB: 1701,
    TUNNEL_DRIVE: 1702,
    RAID: 1703,
    INTIMIDATE: 1704,
    SWORD_MASTERY_ROGUE: 1705,
    VULTURES_EYE_ROGUE: 1706,
    DOUBLE_STRAFE_ROGUE: 1707,
    REMOVE_TRAP_ROGUE: 1708,
    STEAL_COIN: 1709,
    DIVEST_HELM: 1710,
    DIVEST_SHIELD: 1711,
    DIVEST_ARMOR: 1712,
    DIVEST_WEAPON: 1713,
    PLAGIARISM: 1714,
    GANGSTERS_PARADISE: 1715,
    COMPULSION_DISCOUNT: 1716,
    SCRIBBLE: 1717,
    CLOSE_CONFINE: 1718
};
```

### 11.2 Snatcher Auto-Steal Chance Table

```js
const SNATCHER_TRIGGER_CHANCE = [0, 70, 80, 100, 110, 130, 140, 160, 170, 190, 200];
// Index = skill level. Values per 1000. Divide by 10 for percentage.
// Level 0 = 0%, Level 1 = 7%, Level 10 = 20%
```

### 11.3 Tunnel Drive Speed Table

```js
const TUNNEL_DRIVE_SPEED = [0, 26, 32, 38, 44, 50];
// Index = skill level. Values in % of normal movement speed.
// Level 0 = cannot move while hidden
```

### 11.4 Strip Rate Formula Constants

```js
// rate (per 1000) = 50 * (skillLevel + 1) + 2 * (casterDEX - targetDEX)
// duration (ms) = 60000 + 15000 * skillLevel + 500 * (casterDEX - targetDEX)

const STRIP_MONSTER_EFFECTS = {
    weapon: { stat: 'atk', reduction: 0.25 },    // -25% ATK
    shield: { stat: 'hardDef', reduction: 0.15 }, // -15% hard DEF
    armor:  { stat: 'vit', reduction: 0.40 },     // -40% VIT
    helm:   { stat: 'int', reduction: 0.40 }      // -40% INT
};
```

### 11.5 Back Stab Damage Table

```js
const BACKSTAB_ATK_PERCENT = [0, 340, 380, 420, 460, 500, 540, 580, 620, 660, 700];
// Index = skill level. ATK% per hit.
// Formula: 300 + level * 40
```

### 11.6 Raid Damage Table (Pre-Renewal)

```js
const RAID_ATK_PERCENT = [0, 140, 180, 220, 260, 300];
const RAID_STATUS_CHANCE = [0, 13, 16, 19, 22, 25];
// Pre-renewal AoE: 3x3 (150 UE units radius)
```

### 11.7 Intimidate Damage Table

```js
const INTIMIDATE_ATK_PERCENT = [0, 130, 160, 190, 220, 250];
const INTIMIDATE_SP_COST = [0, 13, 16, 19, 22, 25];
```

### 11.8 Steal Coin Zeny Formula

```js
function calculateStolenZeny(monsterLevel) {
    return Math.floor(Math.random() * (2 * monsterLevel + 2)) + 8 * monsterLevel;
}
// Monster Lv50: 400-501 Zeny
// Monster Lv80: 640-801 Zeny
```

### 11.9 Compulsion Discount Table

```js
const COMPULSION_DISCOUNT = [0, 9, 13, 17, 21, 25];
// Index = skill level. Discount % off NPC buy prices.
// Formula: 5 + level * 4
```

### 11.10 Plagiarism ASPD Bonus

```js
// Plagiarism grants +1% ASPD per skill level (Lv10 = +10% ASPD)
// Applied as buffAspdMultiplier addition in getEffectiveStats()
```

---

## 12. Sources

- [iRO Wiki Classic -- Rogue](https://irowiki.org/classic/Rogue)
- [iRO Wiki Classic -- Back Stab](https://irowiki.org/classic/Back_Stab)
- [iRO Wiki Classic -- Sightless Mind](https://irowiki.org/classic/Sightless_Mind)
- [iRO Wiki Classic -- Intimidate (Plagiarism)](https://irowiki.org/classic/Intimidate)
- [iRO Wiki -- Back Stab](https://irowiki.org/wiki/Back_Stab)
- [iRO Wiki -- Mug](https://irowiki.org/wiki/Mug)
- [iRO Wiki -- Stalk](https://irowiki.org/wiki/Stalk)
- [iRO Wiki -- Haggle](https://irowiki.org/wiki/Haggle)
- [iRO Wiki -- Close Confine](https://irowiki.org/wiki/Close_Confine)
- [RateMyServer -- Rogue Skills](https://ratemyserver.net/index.php?page=skill_db&jid=17)
- [RateMyServer -- Back Stab](https://ratemyserver.net/index.php?page=skill_db&skid=212)
- [RateMyServer -- Raid/Sightless Mind](https://ratemyserver.net/index.php?page=skill_db&skid=214)
- [RateMyServer -- Divest Weapon](https://ratemyserver.net/index.php?page=skill_db&skid=215)
- [RateMyServer -- Divest Shield](https://ratemyserver.net/index.php?page=skill_db&skid=216)
- [RateMyServer -- Divest Armor](https://ratemyserver.net/index.php?page=skill_db&skid=217)
- [RateMyServer -- Divest Helm](https://ratemyserver.net/index.php?page=skill_db&skid=218)
- [RateMyServer -- Intimidate/Snatch](https://ratemyserver.net/index.php?page=skill_db&skid=219)
- [RateMyServer -- Plagiarism](https://ratemyserver.net/index.php?page=skill_db&skid=225)
- [RateMyServer -- Gank/Snatcher](https://ratemyserver.net/index.php?page=skill_db&skid=210)
- [RateMyServer -- Steal Coin/Mug](https://ratemyserver.net/index.php?page=skill_db&skid=211)
- [RateMyServer -- Close Confine](https://ratemyserver.net/index.php?page=skill_db&skid=1005)
- [RateMyServer -- Haggle](https://ratemyserver.net/index.php?page=skill_db&skid=224)
- [rAthena Pre-RE DB -- Back Stab](https://pre.pservero.com/skill/RG_BACKSTAP)
- [rAthena Pre-RE DB -- Raid](https://pre.pservero.com/skill/RG_RAID)
- [rAthena Pre-RE DB -- Intimidate](https://pre.pservero.com/skill/RG_INTIMIDATE)
- [rAthena Pre-RE DB -- Divest Weapon](https://pre.pservero.com/skill/RG_STRIPWEAPON)
- [rAthena Pre-RE DB -- Divest Shield](https://pre.pservero.com/skill/RG_STRIPSHIELD)
- [rAthena Pre-RE DB -- Divest Armor](https://pre.pservero.com/skill/RG_STRIPARMOR)
- [rAthena Pre-RE DB -- Divest Helm](https://pre.pservero.com/skill/RG_STRIPHELM)
- [rAthena GitHub Issue #3476 -- Strip rates unofficial](https://github.com/rathena/rathena/issues/3476)
- [DarkHikari's Plagiarism Guide -- Copyable Skills List](https://myrolife.blogspot.com/2016/05/darkhikaris-intimidateplagiarism-guide.html)
- [rAthena GitHub Commit 4a8be87 -- Strip fixes](https://github.com/rathena/rathena/commit/4a8be872150844e8c1acd2e1024d3f055d5a9fe9)
