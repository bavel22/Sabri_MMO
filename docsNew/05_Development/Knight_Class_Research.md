# Knight Class Research -- Complete Pre-Renewal Implementation Reference

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Knight_Skills_Audit](Knight_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Scope:** Knight class (11 skills + 1 Soul Link skill), Peco Peco mount system, all formulas and mechanics
**Sources:** iRO Wiki Classic, iRO Wiki (combined), RateMyServer, rAthena Pre-Renewal DB, Divine Pride, RagnaCloneDocs

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Skill Tree and Prerequisites](#skill-tree-and-prerequisites)
3. [Passive Skills](#passive-skills)
   - 3.1 [Spear Mastery (ID 700)](#spear-mastery-id-700)
   - 3.2 [Riding / Peco Peco Ride (ID 708)](#riding--peco-peco-ride-id-708)
   - 3.3 [Cavalier Mastery (ID 709)](#cavalier-mastery-id-709)
4. [Offensive Skills](#offensive-skills)
   - 4.1 [Pierce (ID 701)](#pierce-id-701)
   - 4.2 [Spear Stab (ID 702)](#spear-stab-id-702)
   - 4.3 [Brandish Spear (ID 703)](#brandish-spear-id-703)
   - 4.4 [Spear Boomerang (ID 704)](#spear-boomerang-id-704)
   - 4.5 [Bowling Bash (ID 707)](#bowling-bash-id-707)
   - 4.6 [Charge Attack (ID 710)](#charge-attack-id-710--quest-skill)
5. [Supportive Skills](#supportive-skills)
   - 5.1 [Two-Hand Quicken (ID 705)](#two-hand-quicken-id-705)
   - 5.2 [Auto Counter (ID 706)](#auto-counter-id-706)
6. [Soul Link Skill](#soul-link-skill)
   - 6.1 [One-Hand Quicken (ID 711)](#one-hand-quicken-id-711--soul-link)
7. [Current Implementation Status](#current-implementation-status)
8. [Skill Definition Corrections](#skill-definition-corrections)
9. [New Systems Required](#new-systems-required)
10. [Implementation Priority](#implementation-priority)
11. [Integration Points](#integration-points)
12. [Constants and Data Tables](#constants-and-data-tables)

---

## Class Overview

| Property | Value |
|----------|-------|
| Base Class | Swordsman |
| Transcendent | Lord Knight |
| Max Job Level | 50 |
| Total Skill Points | 76 |
| Quest Skills | 1 (Charge Attack, Job Lv 40) |
| Soul Link Skill | 1 (One-Hand Quicken, requires Knight Spirit) |
| Weapons | 1H Swords, 2H Swords, Daggers, Spears, Maces, Axes |
| Mount | Peco Peco (via Riding skill) |
| HP Coefficient | HP_JOB_A=1.5, HP_JOB_B=5 (highest HP growth in game) |
| SP Coefficient | SP_JOB=3 |
| ASPD Base Delays | bare_hand=38, dagger=60, 1H_sword=55, 2H_sword=50, spear=55, mace=60, axe=70 |

### Job Stat Bonuses (Level-by-Level)

| Stat | Total | Levels |
|------|-------|--------|
| STR | +8 | 4, 10, 15, 21, 27, 33, 46, 47 |
| AGI | +2 | 13, 38 |
| VIT | +10 | 1, 3, 8, 12, 17, 18, 23, 29, 36, 43 |
| INT | +0 | -- |
| DEX | +6 | 5, 20, 31, 40, 48, 49 |
| LUK | +4 | 5, 20, 28, 37 |

### Inherited Swordsman Skills

Knights inherit all Swordsman skills (IDs 100-109):
- Sword Mastery (100), Two-Handed Sword Mastery (101), Increase HP Recovery (102)
- Bash (103), Provoke (104), Magnum Break (105), Endure (106)
- Moving HP Recovery (107, quest), Auto Berserk (108, quest), Fatal Blow (109, quest)

---

## Skill Tree and Prerequisites

```
                    Swordsman Tree (inherited)
                    ==========================
    Sword Mastery(100) ──> 2H Sword Mastery(101) ──> [Two-Hand Quicken(705)]
    Bash(103) ──> Magnum Break(105)
    Provoke(104) ──> Endure(106) ──> [Riding(708)]

                    Knight Tree
                    ===========
    Spear Mastery(700) ──> Pierce(701) ──> Spear Stab(702) ──> [Brandish Spear(703)]
                       │                │                         (also needs Riding 1)
                       │                └──> Spear Boomerang(704)
                       │
    2H Sword Mastery(101) ──> Two-Hand Quicken(705) ──> Auto Counter(706)
                                                          │
    Bash(103,Lv10) + MB(105,Lv3) + 2HSM(101,Lv5) ────────┤
    + THQ(705,Lv10) + AC(706,Lv5) ──> Bowling Bash(707)

    Endure(106,Lv1) ──> Riding(708) ──> Cavalier Mastery(709)
                                    └──> [Brandish Spear(703)]

    Charge Attack(710) ──> (Quest skill, no tree prereqs)
```

### Full Prerequisite Map

| Skill | Prerequisites |
|-------|--------------|
| Spear Mastery (700) | None |
| Pierce (701) | Spear Mastery Lv1 |
| Spear Stab (702) | Pierce Lv5 |
| Brandish Spear (703) | Spear Stab Lv3, Riding Lv1 |
| Spear Boomerang (704) | Pierce Lv3 |
| Two-Hand Quicken (705) | Two-Handed Sword Mastery Lv1 (Swordsman 101) |
| Auto Counter (706) | Two-Handed Sword Mastery Lv1 (Swordsman 101) |
| Bowling Bash (707) | Bash Lv10, Magnum Break Lv3, Two-Handed Sword Mastery Lv5, Two-Hand Quicken Lv10, Auto Counter Lv5 |
| Riding (708) | Endure Lv1 (Swordsman 106) |
| Cavalier Mastery (709) | Riding Lv1 |
| Charge Attack (710) | Quest skill (Job Lv 40 quest) |
| One-Hand Quicken (711) | Two-Hand Quicken Lv10, Knight Spirit soul link active |

---

## Passive Skills

### Spear Mastery (ID 700)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| Weapon Types | All Spears (1H and 2H) | iRO Wiki Classic, rAthena |
| Mechanic | Mastery ATK (bypasses DEF, added to final damage) | iRO Wiki Classic |
| Prerequisites | None | All sources |
| Classes | Knight, Lord Knight, Crusader, Paladin | iRO Wiki Classic |

#### ATK Bonus Table

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Dismounted | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |
| Mounted | +5 | +10 | +15 | +20 | +25 | +30 | +35 | +40 | +45 | +50 |

**Dismounted formula:** `+4 * SkillLv` ATK
**Mounted formula:** `+5 * SkillLv` ATK (extra +1 per level from Riding synergy)

The mounted bonus comes from Spear Mastery itself -- when the player has Riding active and is mounted, Spear Mastery automatically provides the higher bonus. This is not a separate effect from the Riding skill.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing mounted bonus | HIGH | Currently only applies `+4 * level` (dismounted). Must check `player.isMounted` and apply `+5 * level` instead. |

#### Implementation Notes

In `getPassiveSkillBonuses()`, the spear mastery section needs:
```js
const smLv = skills[700] || 0;
if (smLv > 0 && wType === 'spear') {
    const bonusPerLevel = player.isMounted ? 5 : 4;
    bonuses.bonusATK += smLv * bonusPerLevel;
}
```

---

### Riding / Peco Peco Ride (ID 708)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 1 | All sources |
| Prerequisites | Endure Lv1 (Swordsman 106) | iRO Wiki Classic, rAthena |

#### Effects When Mounted

| Effect | Value | Source |
|--------|-------|--------|
| Movement Speed | +25% base move speed | iRO Wiki Classic, RateMyServer |
| Weight Capacity | +1000 weight limit | iRO Wiki Classic |
| ASPD Penalty | ASPD set to 50% of normal (halved) | iRO Wiki Classic |
| Spear vs Medium | Spear weapons deal 100% damage to Medium size (ignores normal size penalty) | iRO Wiki Classic |
| Spear Mastery Bonus | Enables the higher mounted ATK bonus (+5/lv instead of +4/lv) | iRO Wiki Classic |
| Sprite | Character model changes to mounted sprite | Visual only |

#### Mount/Dismount Mechanics

- Player toggles mount on/off (like a toggle skill)
- Cannot mount indoors in some zones (RO Classic restriction -- may defer)
- Dismounting restores normal ASPD, removes weight bonus, removes mounted Spear Mastery bonus
- The mount is purely cosmetic + stat modifier -- no separate entity

#### Current Implementation Status: DEFINITION ONLY

The skill is defined in `ro_skill_data_2nd.js` (ID 708) but no mount system exists. The `player.isMounted` flag does not exist yet.

---

### Cavalier Mastery (ID 709)

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 5 | All sources |
| Prerequisites | Riding Lv1 (ID 708) | All sources |

#### ASPD Recovery Table

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ASPD Restored | 60% | 70% | 80% | 90% | 100% |

**Formula:** Without Cavalier Mastery, mounted ASPD is 50% of normal. At each level, the penalty is reduced:
- Effective ASPD = `NormalASPD * (0.5 + 0.5 * CavalierMasteryLv / 5)`
- Lv0: 50%, Lv1: 60%, Lv2: 70%, Lv3: 80%, Lv4: 90%, Lv5: 100% (full restore)

Or equivalently: the ASPD multiplier while mounted = `0.5 + CavalierMasteryLv * 0.1`

#### Current Implementation Status: DEFINITION ONLY

Defined in skill data but no ASPD interaction exists since the mount system is not implemented.

---

## Offensive Skills

### Pierce (ID 701)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical | All sources |
| Max Level | 10 | All sources |
| Target | Single Enemy | All sources |
| Range | Melee (2-3 cells with spear) | iRO Wiki Classic, RateMyServer |
| Element | Weapon element (neutral default) | rAthena |
| Weapon Requirement | Spear only | All sources |
| Cast Time | 0 (instant, ASPD-based) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| SP Cost | 7 (flat, all levels) | All sources |
| Prerequisites | Spear Mastery Lv1 | All sources |

#### Damage Table

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% per hit | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 | 200 |
| Accuracy Bonus | +5% | +10% | +15% | +20% | +25% | +30% | +35% | +40% | +45% | +50% |

**Damage formula:** `(100 + 10 * SkillLv)%` ATK per hit
**Accuracy bonus formula:** `+5 * SkillLv` percent (multiplicative HIT bonus, applied after base HIT calculation)

#### Multi-Hit by Target Size (CRITICAL MECHANIC)

| Target Size | Number of Hits |
|-------------|---------------|
| Small | 1 hit |
| Medium | 2 hits |
| Large | 3 hits |

Each hit deals the full ATK% independently (not split). Against Large monsters at Lv10, this deals `200% * 3 = 600%` total ATK damage. Despite multiple hits visually, the damage is applied as a single bundle server-side.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing multi-hit by target size | CRITICAL | Currently deals 1 hit regardless of target size. Must check `target.size` and multiply. |
| Missing accuracy bonus | MEDIUM | +5% per level HIT bonus not applied. |
| Missing spear weapon check | HIGH | Should fail if player does not have a spear equipped. |
| Cooldown set to 500 | MEDIUM | Should be 0 -- no per-skill cooldown, limited by ASPD only. |
| Range set to 300 | LOW | Spear melee range is ~2-3 cells. 300 UE units is acceptable but verify. |

#### Implementation Notes

**Multi-hit handler:**
```js
case 'pierce': {
    // Weapon check
    if (player.weaponType !== 'spear') {
        socket.emit('skill:error', { message: 'Pierce requires a Spear' });
        return;
    }

    const targetSize = enemy.size || 'medium';
    const hitCount = targetSize === 'large' ? 3 : (targetSize === 'medium' ? 2 : 1);

    let totalDamage = 0;
    for (let i = 0; i < hitCount; i++) {
        const result = calculatePhysicalDamage(attackerStats, targetStats, {
            isSkill: true,
            skillMultiplier: effectVal,
            skillHitBonus: learnedLevel * 5, // +5% HIT per level
            forceHit: false
        });
        totalDamage += result.damage;
    }
    // Apply total damage as single bundle
    break;
}
```

---

### Spear Stab (ID 702)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical | All sources |
| Max Level | 10 | All sources |
| Target | Single (line AoE between caster and target) | iRO Wiki Classic |
| Range | Melee (4 cells with spear per RateMyServer) | RateMyServer |
| Element | Weapon element | rAthena |
| Weapon Requirement | Spear only | All sources |
| Cast Time | 0 (ASPD-based) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| SP Cost | 9 (flat, all levels) | All sources |
| Knockback | 6 cells (disabled in WoE) | All sources |
| Prerequisites | Pierce Lv5 | All sources |

#### Damage Table

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

**Damage formula:** `(100 + 20 * SkillLv)%` ATK

#### Line AoE Mechanic (KEY FEATURE)

Spear Stab hits ALL enemies in a straight line between the caster and the targeted enemy. The line is defined by the caster's position and the target's position. All enemies within the line path take the same damage and are knocked back 6 cells.

This is NOT an AoE splash -- it is a directional line pierce. The implementation should:
1. Calculate the line from caster to target
2. Find all enemies along that line (within a narrow width tolerance)
3. Deal damage + knockback to all of them

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing line AoE | HIGH | Currently single-target only. Must hit all enemies between caster and target. |
| Missing knockback | HIGH | 6 cells knockback not implemented. |
| Missing spear weapon check | HIGH | Should fail without spear. |
| Cooldown set to 500 | MEDIUM | Should be 0. |

#### Implementation Notes

**Line AoE detection:**
```js
// Find all enemies along the line from caster to target
function getEnemiesAlongLine(casterPos, targetPos, lineWidth, maxDist) {
    const lineEnemies = [];
    const dx = targetPos.x - casterPos.x;
    const dy = targetPos.y - casterPos.y;
    const lineDist = Math.sqrt(dx * dx + dy * dy);
    if (lineDist === 0) return lineEnemies;

    // Normalize direction
    const nx = dx / lineDist;
    const ny = dy / lineDist;

    for (const [eid, e] of enemies) {
        if (e.isDead || e.zone !== zone) continue;
        // Project enemy position onto line
        const ex = e.x - casterPos.x;
        const ey = e.y - casterPos.y;
        const dot = ex * nx + ey * ny; // distance along line
        if (dot < 0 || dot > maxDist) continue; // behind caster or beyond target
        const perpDist = Math.abs(ex * ny - ey * nx); // perpendicular distance
        if (perpDist <= lineWidth) {
            lineEnemies.push({ enemy: e, enemyId: eid, distance: dot });
        }
    }
    return lineEnemies.sort((a, b) => a.distance - b.distance);
}
```

---

### Brandish Spear (ID 703)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, AoE | All sources |
| Max Level | 10 | All sources |
| Target | Directional AoE (frontal cone from caster) | iRO Wiki Classic, RateMyServer |
| Range | Melee | All sources |
| Element | Weapon element | rAthena |
| Weapon Requirement | Spear (1H or 2H) | All sources |
| Mount Requirement | MUST be mounted on Peco Peco | All sources |
| Cast Time | 700ms (uninterruptible, reduced by DEX in pre-renewal) | iRO Wiki Classic |
| After-Cast Delay | 1000ms | iRO Wiki Classic, RateMyServer |
| Cooldown | 0 | rAthena |
| SP Cost | 12 (flat, all levels) | All sources |
| Knockback | 2 cells (disabled in WoE) | All sources |
| Prerequisites | Spear Stab Lv3, Riding Lv1 | All sources |

#### Damage Table (Pre-Renewal)

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

**Pre-Renewal damage formula:** `(100 + 20 * SkillLv)%` ATK
**Note:** Renewal changed the formula to `(400 + 100 * SkillLv)%` with STR scaling. We use pre-renewal.

#### AoE Pattern (Directional, Level-Dependent)

The AoE is a frontal cone/rectangle pattern extending forward from the caster in the direction of the targeted enemy. It expands at skill levels 4, 7, and 10:

```
Direction: North (example, rotates based on target direction)

Level 10 only:       .DDD.
Levels 7-10:         CCCCC
Levels 4-10:         BBBBB
All levels:          AAAAA
Caster position:     AAXAA

X = caster (Knight)
A = Area 1 (always affected, ~1 cell radius around caster extending forward)
B = Area 2 (added at Lv4+, extends forward 1 more row)
C = Area 3 (added at Lv7+, extends forward 1 more row)
D = Area 4 (added at Lv10 only, extends forward 1 more row, narrower)
```

| Level Range | AoE Depth | AoE Width |
|-------------|-----------|-----------|
| 1-3 | 2 rows | 5 cells wide |
| 4-6 | 3 rows | 5 cells wide |
| 7-9 | 4 rows | 5 cells wide |
| 10 | 5 rows | 5 cells wide (row D is 3 cells) |

**Simplified server AoE approach:** Treat as a rectangular AoE extending forward from the caster:
- Lv1-3: radius ~200 UE units forward, ~250 UE units wide
- Lv4-6: radius ~300 UE units forward
- Lv7-9: radius ~400 UE units forward
- Lv10: radius ~500 UE units forward

#### Special Mechanics

1. **Mount required** -- cannot be used while dismounted. Emit `skill:error` if `!player.isMounted`.
2. **Directional** -- AoE direction determined by caster's facing toward the targeted enemy.
3. **DEF reduction** -- During cast, caster's DEF is reduced to 2/3 of normal (pre-renewal mechanic).
4. **Weapon lock** -- Cannot change weapons during cast/execution.
5. **Single damage bundle** -- All damage applied as one transaction despite hitting multiple targets.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing mount requirement check | CRITICAL | Must verify `player.isMounted` before allowing use. |
| Missing directional AoE pattern | CRITICAL | Current `targetType: 'aoe'` does not implement directional cone. |
| Missing spear weapon check | HIGH | Must verify spear weapon. |
| Wrong SP cost | HIGH | Currently `spCost: 12` is correct. |
| Wrong cast time | MEDIUM | Currently 700ms which matches pre-renewal. |
| Missing DEF reduction during cast | LOW | 2/3 DEF during cast -- may defer. |
| Cooldown set to 1000 | MEDIUM | Should be `afterCastDelay: 1000, cooldown: 0`. |

---

### Spear Boomerang (ID 704)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged | All sources |
| Max Level | 5 | All sources |
| Target | Single Enemy | All sources |
| Element | Weapon element | rAthena |
| Weapon Requirement | Spear (1H or 2H) | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 1000ms (cast delay) | iRO Wiki Classic |
| Cooldown | 0 | rAthena |
| SP Cost | 10 (flat, all levels) | All sources |
| Prerequisites | Pierce Lv3 | All sources |

#### Damage and Range Table

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 150 | 200 | 250 | 300 | 350 |
| Range (cells) | 3 | 5 | 7 | 9 | 11 |
| Range (UE units) | 150 | 250 | 350 | 450 | 550 |

**Damage formula:** `(100 + 50 * SkillLv)%` ATK
**Range formula:** `(3 + 2 * SkillLv)` cells = `(1 + 2 * SkillLv) * 50` UE units

**Key mechanic:** This is a RANGED physical attack despite being a spear skill. It can be blocked by Pneuma and can trigger ranged-specific card effects.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing spear weapon check | HIGH | Must verify spear weapon. |
| Wrong range scaling | MEDIUM | Currently `range: 600` flat. Should scale with level: 150/250/350/450/550 UE units. |
| Missing ranged physical flag | MEDIUM | Should be flagged as ranged attack for Pneuma interaction. |
| Cooldown set to 1000 | MEDIUM | Should be `afterCastDelay: 1000, cooldown: 0`. |

---

### Bowling Bash (ID 707)

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, AoE | All sources |
| Max Level | 10 | All sources |
| Target | Single enemy (with 3x3 AoE splash) | iRO Wiki Classic |
| Range | Melee (2 cells) | All sources |
| Element | Weapon element (neutral default) | rAthena |
| Weapon Requirement | None officially, but optimal with 2H Swords | RateMyServer |
| Cast Time | 700ms (uninterruptible, reduced by DEX) | iRO Wiki Classic |
| After-Cast Delay | ASPD-based | iRO Wiki Classic |
| Cooldown | 0 | rAthena |
| SP Cost | 13-22 (SkillLv + 12) | All sources |
| Knockback | 1 cell (in direction caster last faced) | iRO Wiki Classic |
| Prerequisites | Bash Lv10, Magnum Break Lv3, 2H Sword Mastery Lv5, Two-Hand Quicken Lv10, Auto Counter Lv5 | All sources |

#### Damage Table

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |
| SP | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 |

**Damage formula:** `(100 + 40 * SkillLv)%` ATK

#### Two-Hit Mechanic (CRITICAL)

Bowling Bash always hits **twice** in pre-renewal. Each hit is calculated independently with the full ATK% multiplier. This means at Lv10, the effective total damage is `500% * 2 = 1000%` ATK.

Important interactions with the two-hit mechanic:
- **Lex Aeterna:** Only doubles the FIRST hit's damage (second hit is normal)
- **Card effects:** Apply to BOTH hits independently
- **Critical:** Each hit has its own critical chance roll
- **DEF:** Each hit is reduced by DEF independently

#### AoE Mechanics (Pre-Renewal)

1. The primary target takes the 2 hits and is knocked back 1 cell
2. If the knocked-back target collides with or passes near other enemies, those secondary enemies are hit in a 3x3 cell splash
3. The splash damage uses the same ATK% but may hit only once (not twice) for secondary targets

#### Gutter Line Mechanic (Pre-Renewal Only)

In pre-renewal RO, an invisible grid exists on every map. When Bowling Bash is used on a cell where `X % 40 == 0` or `Y % 40 == 0` (and the 5 adjacent cells), the skill reverts to an older version with different knockback/AoE behavior. This was a known exploit.

**Implementation recommendation:** Do NOT implement gutter lines. This was a bug/unintended behavior that was eventually removed. Our implementation should use the standard 2-hit + 3x3 splash + 1-cell knockback consistently.

#### DEF Reduction During Cast

During the cast animation (700ms), the caster's DEF is reduced to 2/3 of normal. This is a self-imposed penalty.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing two-hit mechanic | CRITICAL | Currently `effectValue: 140+i*40` suggests 1 hit. Must deal 2 separate damage hits. |
| Missing 3x3 AoE splash | HIGH | Knocked-back target should splash damage to nearby enemies. |
| Missing knockback | HIGH | 1 cell knockback in caster's facing direction. |
| Wrong prerequisite: missing MB Lv3 and 2HSM Lv5 | HIGH | Prerequisites are partially set but need verification. |
| Missing cast time | HIGH | 700ms uninterruptible cast not in skill data. |
| Missing DEF reduction during cast | LOW | 2/3 DEF during cast -- may defer. |
| Cooldown set to 500 | MEDIUM | Should be 0 (no per-skill cooldown). |

#### Implementation Notes

**Two-hit damage:**
```js
case 'bowling_bash': {
    const castTime = 700; // ms, uninterruptible, reduced by DEX
    // After cast completes:
    let hit1 = calculatePhysicalDamage(attackerStats, targetStats, {
        isSkill: true, skillMultiplier: effectVal, forceHit: false
    });
    let hit2 = calculatePhysicalDamage(attackerStats, targetStats, {
        isSkill: true, skillMultiplier: effectVal, forceHit: false
    });

    // Lex Aeterna only doubles first hit
    if (targetBuffMods.doubleNextDamage) {
        hit1.damage *= 2;
        removeBuff(target, 'lex_aeterna');
    }

    const totalDamage = hit1.damage + hit2.damage;

    // Knockback target 1 cell in caster's facing direction
    knockbackTarget(enemy, attackerPos.x, attackerPos.y, 1, zone, ...);

    // Check for splash targets in 3x3 around knocked-back position
    const splashTargets = findEnemiesInRadius(newTargetPos, 150, zone);
    for (const splash of splashTargets) {
        const splashResult = calculatePhysicalDamage(attackerStats, splashStats, {
            isSkill: true, skillMultiplier: effectVal, forceHit: false
        });
        // Apply single-hit splash damage
    }
    break;
}
```

---

### Charge Attack (ID 710) -- Quest Skill

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, Physical, Ranged | All sources |
| Max Level | 1 | All sources |
| Target | Single Enemy | All sources |
| Range | 14 cells (~700 UE units) | All sources |
| Element | Weapon element | rAthena |
| Weapon Requirement | None (any weapon) | RateMyServer |
| Quest Requirement | Knight Platinum Skills Quest, Job Lv 40 | iRO Wiki Classic |
| Knockback | 1 cell (pre-renewal) | iRO Wiki Classic |

#### Distance-Based Damage (Pre-Renewal)

| Distance | Damage | Cast Time |
|----------|--------|-----------|
| 0-2 cells | 100% ATK | 0.5s |
| 3-5 cells | 200% ATK | 0.7s |
| 6-8 cells | 300% ATK | 1.0s |
| 9-11 cells | 400% ATK | 1.2s |
| 12-14 cells | 500% ATK | 1.5s |

**SP Cost:** 40 (flat)

#### Special Mechanics

1. **Distance-based scaling** -- Damage and cast time both increase with distance to target.
2. **Movement** -- The caster rushes to the target's position regardless of hit/miss.
3. **Ranged physical** -- Despite moving to the target, the damage is classified as ranged (blocked by Pneuma).
4. **Trap escape** -- Can escape Ankle Snare, Fiber Lock, and Close Confine if an enemy is in range.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing distance-based damage scaling | CRITICAL | Currently flat `effectValue: 300`. Must calculate based on caster-target distance. |
| Missing distance-based cast time | HIGH | Cast time should vary: 0.5-1.5s based on distance. |
| Missing caster movement | MEDIUM | Caster should teleport/rush to target position. |
| Missing knockback | MEDIUM | 1 cell knockback on target. |
| Range set to 900 | MEDIUM | Should be ~700 UE units (14 cells). |

#### Implementation Notes

```js
case 'charge_attack': {
    const dx = targetPos.x - attackerPos.x;
    const dy = targetPos.y - attackerPos.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    const cellDist = Math.floor(dist / 50); // Convert UE units to cells

    let damageMultiplier, castTime;
    if (cellDist < 3) { damageMultiplier = 100; castTime = 500; }
    else if (cellDist < 6) { damageMultiplier = 200; castTime = 700; }
    else if (cellDist < 9) { damageMultiplier = 300; castTime = 1000; }
    else if (cellDist < 12) { damageMultiplier = 400; castTime = 1200; }
    else { damageMultiplier = 500; castTime = 1500; }

    // After cast completes, move caster to target and deal damage
    break;
}
```

---

## Supportive Skills

### Two-Hand Quicken (ID 705)

| Property | Value | Source |
|----------|-------|--------|
| Type | Supportive, Self Buff | All sources |
| Max Level | 10 | All sources |
| Target | Self | All sources |
| Weapon Requirement | Two-Handed Sword MUST be equipped | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 0 | rAthena |
| Prerequisites | Two-Handed Sword Mastery Lv1 (Swordsman 101) | All sources |

#### Per-Level Table

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP Cost | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| Duration (s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

**SP formula:** `10 + SkillLv * 4`
**Duration formula:** `SkillLv * 30` seconds

#### ASPD Bonus (Pre-Renewal ONLY)

In **pre-renewal**, Twohand Quicken provides:
- **+30% ASPD** (applied as `buffAspdMultiplier *= 1.3`)

In **renewal** (NOT our target), it additionally provides CRI (+3 to +12) and HIT (+2 to +20) bonuses. These DO NOT exist in pre-renewal.

**Important:** The existing skill definition correctly has `effectValue: 30` (30% ASPD) and the buff system already handles `two_hand_quicken` in `getBuffModifiers()` with `aspdMultiplier *= (1 + buff.aspdIncrease / 100)`. This is correct for pre-renewal.

#### Cancellation Conditions

The buff is removed by:
1. **Decrease AGI** (Acolyte skill)
2. **Quagmire** (Wizard ground skill)
3. **Unequipping the 2H Sword** -- must check weapon type on equip change
4. Manual cancelation (right-click buff icon)

#### Current Implementation Status: BUFF SYSTEM READY, NO SKILL HANDLER

The buff system in `ro_buff_system.js` already handles `two_hand_quicken` (line 353-355). However, no `skill:use` handler exists in `index.js` to apply the buff when the skill is cast.

#### Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| Missing skill handler | CRITICAL | No handler in `index.js` to apply the buff. |
| Missing weapon type check | HIGH | Must verify `player.weaponType === 'two_hand_sword'` before cast. |
| Missing cancellation on weapon change | HIGH | Must remove buff when 2H sword is unequipped. |
| Missing Decrease AGI / Quagmire cancellation | MEDIUM | These debuffs should remove the buff. |

#### Implementation Notes

```js
case 'two_hand_quicken': {
    // Weapon check
    if (player.weaponType !== 'two_hand_sword') {
        socket.emit('skill:error', { message: 'Two-Hand Quicken requires a Two-Handed Sword' });
        return;
    }

    // SP deduction
    player.mana = Math.max(0, player.mana - spCost);

    // Apply buff
    const duration = learnedLevel * 30000; // 30s per level
    applyBuff(player, {
        skillId: 705,
        name: 'two_hand_quicken',
        casterId: characterId,
        casterName: player.characterName,
        aspdIncrease: 30, // +30% ASPD
        duration: duration
    });

    // Broadcast
    broadcastToZone(zone, 'skill:buff_applied', {
        targetId: characterId, targetName: player.characterName, isEnemy: false,
        casterId: characterId, casterName: player.characterName,
        skillId: 705, buffName: 'Two-Hand Quicken', duration: duration,
        effects: { aspdIncrease: 30 }
    });

    socket.emit('skill:used', { skillId, skillName: 'Two-Hand Quicken', level: learnedLevel,
        spCost, remainingMana: player.mana, maxMana: player.maxMana });
    break;
}
```

**Weapon change cancellation** -- in the equip handler:
```js
// When player changes weapon, check if Two-Hand Quicken should be cancelled
if (hasBuff(player, 'two_hand_quicken') && player.weaponType !== 'two_hand_sword') {
    removeBuff(player, 'two_hand_quicken');
    broadcastToZone(zone, 'skill:buff_removed', {
        targetId: characterId, isEnemy: false, buffName: 'two_hand_quicken', reason: 'weapon_change'
    });
}
```

---

### Auto Counter (ID 706)

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, Self Stance (Counter) | All sources |
| Max Level | 5 | All sources |
| Target | Self (enters guard stance) | All sources |
| Weapon Requirement | Melee weapon (cannot use with bows) | RateMyServer |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | ASPD-based | iRO Wiki Classic |
| Cooldown | 0 | rAthena |
| SP Cost | 3 (flat, all levels) | All sources |
| Prerequisites | Two-Handed Sword Mastery Lv1 (Swordsman 101) | All sources |

#### Stance Duration Table

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration (ms) | 400 | 800 | 1200 | 1600 | 2000 |

**Duration formula:** `SkillLv * 400` ms (0.4s per level)

#### Counter Mechanics (Pre-Renewal)

**Passive Counter (auto-trigger):**
1. When a melee physical attack hits the caster during the stance window:
   - The attack is **blocked** (no damage taken)
   - The caster performs a **critical hit** counter-attack on the attacker
   - The stance ends immediately
   - The critical counter ignores DEF
2. The caster must be **facing** the attacker for the counter to trigger
3. **Skills cannot be countered** -- only normal auto-attacks
4. Only one counter per stance activation

**Active Counter (click-to-attack):**
- If the player clicks a monster while in Auto Counter stance, they attack with:
  - Double critical chance (2x normal crit rate)
  - +20 accuracy bonus
- This also ends the stance

#### Directional Requirement

The caster must be facing toward the incoming attack source. If attacked from behind or the side, the counter does not trigger and the caster takes damage normally.

#### Current Implementation Gaps

| Gap | Priority | Details |
|-----|----------|---------|
| No stance system exists | CRITICAL | Auto Counter requires a time-limited "stance" state on the player. |
| No counter-trigger on incoming melee attack | CRITICAL | Combat tick must check if target has Auto Counter stance and is facing attacker. |
| Missing guaranteed critical on counter | HIGH | Counter-attack must always crit and ignore DEF. |
| Missing directional check | MEDIUM | Must verify caster is facing the attacker (may simplify to "always triggers" for v1). |
| No active counter mechanic | LOW | Click-to-attack with 2x crit. Lower priority. |
| Cooldown set to 500 | MEDIUM | Should be 0. |

#### Implementation Notes

**Stance system:**
```js
case 'auto_counter': {
    // Set stance flag on player
    player.autoCounterStance = {
        active: true,
        expiresAt: Date.now() + (learnedLevel * 400),
        skillLevel: learnedLevel
    };

    player.mana = Math.max(0, player.mana - spCost);
    socket.emit('skill:used', { ... });

    // Set timeout to clear stance
    setTimeout(() => {
        if (player.autoCounterStance && player.autoCounterStance.active) {
            player.autoCounterStance = null;
            // Notify client stance ended
        }
    }, learnedLevel * 400);
    break;
}
```

**In combat tick (when enemy attacks player):**
```js
// Check Auto Counter stance before applying damage
if (target.autoCounterStance && target.autoCounterStance.active && Date.now() < target.autoCounterStance.expiresAt) {
    // Block the incoming damage
    // Counter-attack the enemy with forced critical
    const counterResult = calculatePhysicalDamage(targetStats, enemyStats, {
        forceCrit: true, forceHit: true, isSkill: false
    });
    // Apply counter damage to enemy
    // Clear stance
    target.autoCounterStance = null;
    // Skip applying damage to the player
    continue;
}
```

---

## Soul Link Skill

### One-Hand Quicken (ID 711) -- Soul Link

| Property | Value | Source |
|----------|-------|--------|
| Type | Supportive, Self Buff | All sources |
| Max Level | 1 | All sources |
| Target | Self | All sources |
| Weapon Requirement | One-Handed Sword MUST be equipped | All sources |
| SP Cost | 100 | iRO Wiki, RateMyServer |
| Duration | 300 seconds (5 minutes) | iRO Wiki |
| Prerequisite | Two-Hand Quicken Lv10, Knight Spirit soul link active | All sources |

#### Effect

Identical to Two-Hand Quicken but for One-Handed Swords:
- **+30% ASPD** (same as Two-Hand Quicken)
- Allows shield usage (since weapon is 1H)

#### Cancellation Conditions

Same as Two-Hand Quicken:
1. Decrease AGI
2. Quagmire
3. Unequipping the 1H Sword
4. Soul Link expiring
5. ASPD potions (cancels them / is cancelled by them -- source conflict, defer)

#### Implementation Priority: LOW

Soul Link is a Transcendent-class system (Soul Linker class). This should be deferred until Soul Linker is implemented. When ready, add to `ro_skill_data_2nd.js`:

```js
{ id: 711, name: 'one_hand_quicken', displayName: 'One-Hand Quicken', classId: 'knight', maxLevel: 1,
  type: 'active', targetType: 'self', element: 'neutral', range: 0,
  description: '+30% ASPD with 1H Swords. Requires Knight Spirit.',
  icon: 'one_hand_quicken', treeRow: 2, treeCol: 3,
  prerequisites: [{ skillId: 705, level: 10 }], soulLinkRequired: true,
  levels: [{ level: 1, spCost: 100, castTime: 0, cooldown: 0, effectValue: 30, duration: 300000 }] }
```

---

## Current Implementation Status

### Skill Data Definitions (`ro_skill_data_2nd.js`)

| ID | Skill | Defined? | Data Correct? | Notes |
|----|-------|----------|---------------|-------|
| 700 | Spear Mastery | Yes | PARTIAL | Missing mounted bonus logic |
| 701 | Pierce | Yes | PARTIAL | Missing multi-hit by size, accuracy bonus, weapon check |
| 702 | Spear Stab | Yes | PARTIAL | Missing line AoE, knockback, weapon check |
| 703 | Brandish Spear | Yes | PARTIAL | Missing mount check, directional AoE pattern |
| 704 | Spear Boomerang | Yes | PARTIAL | Missing range scaling, weapon check |
| 705 | Two-Hand Quicken | Yes | CORRECT | SP and duration formulas match pre-renewal |
| 706 | Auto Counter | Yes | PARTIAL | Duration formula correct, but stance system missing |
| 707 | Bowling Bash | Yes | PARTIAL | Missing 2-hit, cast time, knockback |
| 708 | Riding | Yes | DEFINITION ONLY | No mount system implemented |
| 709 | Cavalier Mastery | Yes | DEFINITION ONLY | No ASPD mount interaction |
| 710 | Charge Attack | Yes | WRONG | Flat 300% instead of distance-based 100-500% |
| 711 | One-Hand Quicken | NO | -- | Not in codebase (Soul Link, low priority) |

### Skill Handlers (`index.js`)

**NONE of the 11 Knight skills have handlers in index.js.** Zero handlers implemented. All Knight skills will need new handler code.

### Buff System (`ro_buff_system.js`)

| Buff | Handled? | Notes |
|------|----------|-------|
| `two_hand_quicken` | YES | Line 353-355: `aspdMultiplier *= (1 + buff.aspdIncrease / 100)` |
| `one_hand_quicken` | NO | Not yet (Soul Link, low priority) |
| `auto_counter` | NO | Needs stance system, not a buff |

### Passive System (`getPassiveSkillBonuses()`)

| Passive | Handled? | Notes |
|---------|----------|-------|
| `spear_mastery` | YES (partial) | Only +4/lv (dismounted). Missing +5/lv (mounted). |
| `riding` | NO | No mount system. |
| `cavalier_mastery` | NO | No ASPD mount penalty system. |

---

## Skill Definition Corrections

Changes needed in `ro_skill_data_2nd.js`:

| ID | Field | Current | Correct | Notes |
|----|-------|---------|---------|-------|
| 701 | cooldown | 500 | 0 | No per-skill cooldown |
| 702 | cooldown | 500 | 0 | No per-skill cooldown |
| 703 | spCost | 12 | 12 | Correct |
| 703 | castTime | 700 | 700 | Correct |
| 703 | cooldown | 1000 | 0 | Move to afterCastDelay |
| 703 | (add) afterCastDelay | -- | 1000 | New field needed |
| 704 | range | 600 | varies | Should be level-dependent: 150/250/350/450/550 |
| 704 | cooldown | 1000 | 0 | Move to afterCastDelay |
| 704 | (add) afterCastDelay | -- | 1000 | New field needed |
| 706 | cooldown | 500 | 0 | No per-skill cooldown |
| 707 | cooldown | 500 | 0 | No per-skill cooldown |
| 707 | (add) castTime | -- | 700 | 700ms uninterruptible |
| 707 | prerequisites | current | add MB Lv3 + 2HSM Lv5 | Missing Magnum Break Lv3 (105,3) and 2HSM Lv5 (101,5) |
| 710 | effectValue | 300 | varies | Must be distance-based: 100-500% |
| 710 | castTime | 1000 | varies | Must be distance-based: 500-1500ms |
| 710 | range | 900 | 700 | 14 cells = ~700 UE units |

### Bowling Bash Prerequisites Fix

Current in codebase:
```js
prerequisites: [{ skillId: 103, level: 10 }, { skillId: 705, level: 10 }, { skillId: 706, level: 5 }]
```

Correct (must add Magnum Break Lv3 and 2H Sword Mastery Lv5):
```js
prerequisites: [
    { skillId: 103, level: 10 },  // Bash Lv10
    { skillId: 105, level: 3 },   // Magnum Break Lv3
    { skillId: 101, level: 5 },   // Two-Handed Sword Mastery Lv5
    { skillId: 705, level: 10 },  // Two-Hand Quicken Lv10
    { skillId: 706, level: 5 }    // Auto Counter Lv5
]
```

---

## New Systems Required

### System 1: Peco Peco Mount System (HIGH PRIORITY for Brandish Spear)

**Scope:** Player mount/dismount toggle, stat modifiers while mounted

**Player State:**
```js
player.isMounted = false; // boolean flag
```

**Mount Toggle Handler (could be bound to Riding skill or a dedicated button):**
```js
function toggleMount(player, characterId, zone) {
    // Check: player must have Riding skill (ID 708)
    const ridingLevel = player.learnedSkills?.[708] || 0;
    if (ridingLevel <= 0) return { error: 'You have not learned Riding' };

    player.isMounted = !player.isMounted;

    if (player.isMounted) {
        // Apply mount bonuses
        player.mountWeightBonus = 1000; // +1000 weight capacity
        player.mountSpeedBonus = 25;    // +25% move speed
        recalculateWeight(player);       // Update weight thresholds
    } else {
        // Remove mount bonuses
        player.mountWeightBonus = 0;
        player.mountSpeedBonus = 0;
        recalculateWeight(player);
    }

    // Broadcast mount state to zone (for sprite change)
    broadcastToZone(zone, 'player:mount_toggle', {
        characterId, isMounted: player.isMounted
    });

    // Recalculate stats (ASPD changes)
    broadcastStatsUpdate(player, characterId, zone);
}
```

**ASPD Integration:**

In `getEffectiveStats()` or `calculateDerivedStats()`:
```js
// Mount ASPD penalty
let mountedAspdMultiplier = 1.0;
if (player.isMounted) {
    const cavalierLv = player.learnedSkills?.[709] || 0;
    // Base penalty: ASPD at 50%
    // Cavalier Mastery restores: 60/70/80/90/100%
    mountedAspdMultiplier = 0.5 + (cavalierLv * 0.1); // 0.5 at Lv0, 1.0 at Lv5
}
stats.buffAspdMultiplier = (stats.buffAspdMultiplier || 1) * mountedAspdMultiplier;
```

**Spear Size Penalty Override:**

In `calculatePhysicalDamage()`, when mounted with spear:
```js
// Mounted spear ignores Medium size penalty (treats Medium as 100%)
if (attacker.isMounted && weaponType === 'spear' && targetSize === 'medium') {
    sizePenaltyPct = 100; // Override normal spear vs medium penalty
}
```

**Spear Mastery Mounted Bonus:**

In `getPassiveSkillBonuses()`:
```js
const spearMasteryLv = skills[700] || 0;
if (spearMasteryLv > 0 && wType === 'spear') {
    const bonusPerLevel = player.isMounted ? 5 : 4;
    bonuses.bonusATK += spearMasteryLv * bonusPerLevel;
}
```

### System 2: Auto Counter Stance System

**Scope:** Time-limited defensive stance with counter-trigger

**Player State:**
```js
player.autoCounterStance = null; // or { active: true, expiresAt: number, skillLevel: number }
```

**Activation (in skill:use handler):**
```js
player.autoCounterStance = {
    active: true,
    expiresAt: Date.now() + (learnedLevel * 400),
    skillLevel: learnedLevel
};
```

**Trigger (in enemy combat tick, before damage is applied to player):**
```js
if (target.autoCounterStance && target.autoCounterStance.active
    && Date.now() < target.autoCounterStance.expiresAt) {

    // Block the incoming attack (skip damage to player)
    // Perform critical counter-attack on the enemy
    const counterResult = calculatePhysicalDamage(playerAsAttacker, enemyAsTarget, {
        forceCrit: true,
        forceHit: true,
        isSkill: false
    });

    // Apply counter damage to enemy
    enemy.health -= counterResult.damage;

    // Clear stance
    target.autoCounterStance = null;

    // Broadcast counter-attack
    broadcastToZone(zone, 'skill:effect_damage', {
        skillId: 706, skillName: 'Auto Counter',
        casterId: characterId, targetId: enemyId, isEnemy: true,
        damage: counterResult.damage, isCritical: true,
        element: counterResult.element
    });

    continue; // Skip normal damage application to player
}
```

**Expiration (automatic):**
```js
// Timer-based cleanup
setTimeout(() => {
    if (player.autoCounterStance && Date.now() >= player.autoCounterStance.expiresAt) {
        player.autoCounterStance = null;
    }
}, learnedLevel * 400 + 100);
```

### System 3: Pierce Multi-Hit by Target Size

**Scope:** Size-dependent hit count for Pierce skill

This is a targeted enhancement to the Pierce skill handler, not a general system. The handler must:
1. Look up `target.size` (from monster template: 'small', 'medium', 'large')
2. Set hit count accordingly (1/2/3)
3. Calculate damage independently for each hit
4. Apply all damage as a single server transaction

### System 4: Line AoE Detection (for Spear Stab)

**Scope:** Find all enemies along a line between two points

See the `getEnemiesAlongLine()` function in the Spear Stab section above. This utility could also be reused for other line-based skills.

### System 5: Directional AoE (for Brandish Spear)

**Scope:** Frontal cone/rectangle AoE extending from caster in a specific direction

```js
function getEnemiesInDirectionalAoE(casterPos, targetPos, depth, width, zone) {
    const results = [];
    const dx = targetPos.x - casterPos.x;
    const dy = targetPos.y - casterPos.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist === 0) return results;

    // Normalize direction
    const nx = dx / dist;
    const ny = dy / dist;

    // Perpendicular direction
    const px = -ny;
    const py = nx;

    for (const [eid, e] of enemies) {
        if (e.isDead || e.zone !== zone) continue;

        // Vector from caster to enemy
        const ex = e.x - casterPos.x;
        const ey = e.y - casterPos.y;

        // Project onto forward direction
        const forwardDist = ex * nx + ey * ny;
        if (forwardDist < -50 || forwardDist > depth) continue; // Behind caster or too far

        // Project onto perpendicular
        const perpDist = Math.abs(ex * px + ey * py);
        if (perpDist > width / 2) continue; // Too far to the side

        results.push({ enemy: e, enemyId: eid });
    }
    return results;
}
```

### System 6: Bowling Bash Two-Hit + Splash

**Scope:** Two independent damage calculations + knockback + 3x3 splash

Not a generic system -- specific to Bowling Bash handler. See the Bowling Bash implementation notes above.

### System 7: Weapon Type Validation for Skills

**Scope:** Generic weapon check before skill execution

Many Knight skills require specific weapon types. Add a weapon validation helper:

```js
// Weapon type requirements per skill
const SKILL_WEAPON_REQUIREMENTS = {
    701: ['spear'],                    // Pierce
    702: ['spear'],                    // Spear Stab
    703: ['spear'],                    // Brandish Spear
    704: ['spear'],                    // Spear Boomerang
    705: ['two_hand_sword'],           // Two-Hand Quicken
    // Auto Counter: any melee (no bows) -- handled separately
    // Bowling Bash: no restriction officially
    // Charge Attack: no restriction
};

function validateWeaponForSkill(player, skillId) {
    const required = SKILL_WEAPON_REQUIREMENTS[skillId];
    if (!required) return true;
    return required.includes(player.weaponType);
}
```

### System 8: Quicken Buff Cancellation on Weapon Change

**Scope:** Remove Two-Hand Quicken / One-Hand Quicken when weapon type changes

In the equipment change handler (`inventory:equip`):
```js
// After weapon change:
if (hasBuff(player, 'two_hand_quicken') && player.weaponType !== 'two_hand_sword') {
    removeBuff(player, 'two_hand_quicken');
    broadcastToZone(zone, 'skill:buff_removed', {
        targetId: characterId, isEnemy: false, buffName: 'two_hand_quicken', reason: 'weapon_change'
    });
}
if (hasBuff(player, 'one_hand_quicken') && player.weaponType !== 'one_hand_sword') {
    removeBuff(player, 'one_hand_quicken');
    broadcastToZone(zone, 'skill:buff_removed', {
        targetId: characterId, isEnemy: false, buffName: 'one_hand_quicken', reason: 'weapon_change'
    });
}
```

---

## Implementation Priority

### Phase A: Core Offensive Skills (Highest Impact)

1. **Two-Hand Quicken handler** -- Most impactful Knight skill, buff system already supports it
2. **Pierce handler** -- Core spear skill, needs multi-hit by size
3. **Spear Boomerang handler** -- Only ranged Knight skill, simple implementation
4. **Bowling Bash handler** -- Signature skill, needs 2-hit + cast time

### Phase B: Skill Data Corrections

5. **Fix Bowling Bash prerequisites** -- Add Magnum Break Lv3 and 2HSM Lv5
6. **Fix cooldown vs afterCastDelay** -- Multiple skills have wrong field
7. **Fix Charge Attack** -- Distance-based damage instead of flat 300%
8. **Add weapon type validation** -- Spear checks for Pierce/Stab/Brandish/Boomerang

### Phase C: AoE and Knockback Skills

9. **Spear Stab handler** -- Line AoE + knockback
10. **Brandish Spear handler** -- Directional AoE + mount check

### Phase D: Stance and Mount Systems

11. **Auto Counter stance system** -- Counter-trigger in combat tick
12. **Peco Peco mount system** -- Mount toggle, ASPD penalty, weight bonus

### Phase E: Passive Integration

13. **Spear Mastery mounted bonus** -- +5/lv when mounted (requires mount system)
14. **Cavalier Mastery ASPD** -- Restore ASPD when mounted (requires mount system)
15. **Charge Attack handler** -- Distance-based damage + caster movement

### Phase F: Soul Link (Deferred)

16. **One-Hand Quicken** -- Deferred until Soul Linker class is implemented

---

## Integration Points

### With Existing Combat System (`calculatePhysicalDamage`)

- Pierce: pass `skillHitBonus: learnedLevel * 5` for accuracy bonus
- All skills: pass `skillMultiplier: effectVal`, `skillElement: null` (weapon element)
- Auto Counter: pass `forceCrit: true, forceHit: true`
- Bowling Bash: call `calculatePhysicalDamage` twice (two hits)

### With Existing Buff System (`ro_buff_system.js`)

- Two-Hand Quicken: already handled (line 353-355), just needs handler to apply buff
- One-Hand Quicken: add same pattern as two_hand_quicken
- Decrease AGI / Quagmire: must cancel quicken buffs (add to those handlers)

### With Status Effect System

- Bowling Bash knockback: use existing `knockbackTarget()` function
- Spear Stab knockback: use existing `knockbackTarget()` for each target in line
- Brandish Spear knockback: use existing `knockbackTarget()` for AoE targets

### With Weight System

- Riding: adds 1000 to max weight (use existing `recalculateWeight()`)
- No new weight interactions for offensive skills

### With ASPD System (`calculateASPD`)

- Two-Hand Quicken: `buffAspdMultiplier` already feeds into ASPD calc
- Mounted ASPD penalty: multiply `buffAspdMultiplier` by mount penalty factor
- Cavalier Mastery: adjust the mount penalty factor

### With Existing Passive System (`getPassiveSkillBonuses`)

- Spear Mastery: already partially handled (line for spear ATK bonus exists)
- Need to add `player.isMounted` check for mounted bonus

### With Client-Side Systems

- Mount sprite: `player:mount_toggle` event for BP_MMOCharacter sprite change
- Auto Counter stance: `skill:stance_start/end` events for visual indicator
- Bowling Bash 2-hit: extend `skill:effect_damage` to support multi-hit display
- Knockback: existing `combat:knockback` event or position update

---

## Constants and Data Tables

### Knight Skill IDs

```js
const KNIGHT_SKILL_IDS = {
    SPEAR_MASTERY: 700,
    PIERCE: 701,
    SPEAR_STAB: 702,
    BRANDISH_SPEAR: 703,
    SPEAR_BOOMERANG: 704,
    TWO_HAND_QUICKEN: 705,
    AUTO_COUNTER: 706,
    BOWLING_BASH: 707,
    RIDING: 708,
    CAVALIER_MASTERY: 709,
    CHARGE_ATTACK: 710,
    ONE_HAND_QUICKEN: 711  // Soul Link skill
};
```

### Weapon Type Requirements

```js
const KNIGHT_WEAPON_REQUIREMENTS = {
    701: { types: ['spear'], error: 'Pierce requires a Spear' },
    702: { types: ['spear'], error: 'Spear Stab requires a Spear' },
    703: { types: ['spear'], error: 'Brandish Spear requires a Spear' },
    704: { types: ['spear'], error: 'Spear Boomerang requires a Spear' },
    705: { types: ['two_hand_sword'], error: 'Two-Hand Quicken requires a Two-Handed Sword' },
    711: { types: ['one_hand_sword'], error: 'One-Hand Quicken requires a One-Handed Sword' }
};
```

### Pierce Hit Count by Size

```js
const PIERCE_HITS_BY_SIZE = {
    small: 1,
    medium: 2,
    large: 3
};
```

### Spear Boomerang Range by Level

```js
const SPEAR_BOOMERANG_RANGE = [150, 250, 350, 450, 550]; // UE units, index = level-1
```

### Charge Attack Distance Brackets

```js
const CHARGE_ATTACK_BRACKETS = [
    { maxCells: 2,  damage: 100, castTime: 500 },
    { maxCells: 5,  damage: 200, castTime: 700 },
    { maxCells: 8,  damage: 300, castTime: 1000 },
    { maxCells: 11, damage: 400, castTime: 1200 },
    { maxCells: 14, damage: 500, castTime: 1500 }
];
```

### Bowling Bash Knockback by Level (Pre-Renewal)

The knockback distance scales with level in pre-renewal:

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Knockback (cells) | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 |

**Formula:** `ceil(SkillLv / 2)` cells

```js
const BOWLING_BASH_KNOCKBACK = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5]; // index = level-1
```

**Note:** Some sources report a flat 1-cell knockback for pre-renewal. The level-based scaling may be from iRO specifically. For our implementation, recommend starting with **1 cell flat** and adjusting if needed.

### Brandish Spear AoE Depth by Level

```js
const BRANDISH_SPEAR_DEPTH = {
    // UE units forward from caster
    1: 200, 2: 200, 3: 200,    // Area A only
    4: 300, 5: 300, 6: 300,    // Areas A+B
    7: 400, 8: 400, 9: 400,    // Areas A+B+C
    10: 500                     // Areas A+B+C+D
};
const BRANDISH_SPEAR_WIDTH = 250; // 5 cells wide = ~250 UE units
```

### Cavalier Mastery ASPD Multiplier

```js
const CAVALIER_MASTERY_ASPD = [0.60, 0.70, 0.80, 0.90, 1.00]; // index = level-1
// Without Cavalier Mastery (level 0): 0.50
// Formula: 0.5 + level * 0.1
```

### Mount System Constants

```js
const MOUNT_CONSTANTS = {
    WEIGHT_BONUS: 1000,        // +1000 weight capacity when mounted
    SPEED_BONUS: 25,           // +25% movement speed when mounted
    ASPD_PENALTY: 0.50,        // ASPD reduced to 50% without Cavalier Mastery
    SPEAR_MEDIUM_OVERRIDE: true // Spears deal 100% to Medium while mounted
};
```

---

## Sources

### iRO Wiki Classic (Pre-Renewal Primary)
- [Knight class](https://irowiki.org/classic/Knight) -- job bonuses, skill points, overview
- [Pierce](https://irowiki.org/classic/Pierce) -- multi-hit by size, accuracy bonus
- [Bowling Bash](https://irowiki.org/classic/Bowling_Bash) -- 2 hits, gutter lines, knockback
- [Brandish Spear](https://irowiki.org/classic/Brandish_Spear) -- directional AoE, mount req
- [Twohand Quicken](https://irowiki.org/classic/Twohand_Quicken) -- ASPD only (no CRI/HIT in pre-RE)
- [Counter Attack](https://irowiki.org/classic/Counter_Attack) -- stance duration, counter mechanics
- [Spear Boomerang](https://irowiki.org/classic/Spear_Boomerang) -- range scaling, damage
- [Spear Stab](https://irowiki.org/classic/Spear_Stab) -- line AoE, knockback 6 cells
- [Spear Mastery](https://irowiki.org/classic/Spear_Mastery) -- mounted bonus source
- [Charge Attack](https://irowiki.org/classic/Charge_Attack) -- distance-based damage

### iRO Wiki (Combined)
- [Knight](https://irowiki.org/wiki/Knight) -- job stat bonus table
- [Bowling Bash](https://irowiki.org/wiki/Bowling_Bash) -- post-2020 update changes
- [Twohand Quicken](https://irowiki.org/wiki/Twohand_Quicken) -- full level tables
- [Counter Attack](https://irowiki.org/wiki/Counter_Attack) -- critical mechanics
- [Charge Attack](https://irowiki.org/wiki/Charge_Attack) -- renewal 700% flat (not our target)
- [Peco Peco Ride](https://irowiki.org/wiki/Peco_Peco_Ride) -- mount effects
- [Cavalier Mastery](https://irowiki.org/wiki/Cavalier_Mastery) -- ASPD recovery table
- [One-Hand Quicken](https://irowiki.org/wiki/One-Hand_Quicken) -- Soul Link skill

### RateMyServer
- [Knight skills page](https://ratemyserver.net/index.php?page=skill_db&jid=7) -- all 12 skills
- [Brandish Spear](https://ratemyserver.net/index.php?page=skill_db&skid=57) -- AoE pattern diagram
- [Bowling Bash](https://ratemyserver.net/index.php?page=skill_db&skid=62) -- knockback table
- [Twohand Quicken](https://ratemyserver.net/index.php?page=skill_db&skid=60) -- pre-RE ASPD only
- [Charge Attack](https://ratemyserver.net/index.php?page=skill_db&skid=1001) -- distance scaling

### Divine Pride
- [Bowling Bash](https://www.divine-pride.net/database/skill/62/bowling-bash) -- 2-hit confirmation

### Existing Codebase
- `server/src/ro_skill_data_2nd.js` (lines 12-23) -- current Knight skill definitions
- `server/src/ro_buff_system.js` (lines 353-355) -- existing `two_hand_quicken` buff handler
- `server/src/ro_damage_formulas.js` -- `calculatePhysicalDamage()`, `calculateASPD()`, size penalty
- `server/src/ro_exp_tables.js` (line 462) -- Knight HP/SP coefficients, ASPD base delays
- `server/src/index.js` -- no Knight skill handlers exist yet
- `docsNew/05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md` -- parent class audit
- `docsNew/05_Development/2nd_Job_Skills_Complete_Research.md` -- existing research summary
- `RagnaCloneDocs/03_Skills_Complete.md` (lines 1056-1211) -- Knight skill reference data

---

## Implementation Audit Notes (2026-03-16)

Deep-research audit completed against 4 sources (iRO Wiki Classic, RateMyServer, Divine Pride, rAthena pre-renewal source code). All 21 issues found were fixed. Key corrections discovered during this process:

1. **Pierce damage is BUNDLED (single packet x size hits), not independent rolls.** rAthena uses `Hit: Multi_Hit` with size-based div set at runtime. All hits appear as one damage number. The previous implementation calculated damage independently for each hit (2-3 times for Med/Large), which was wrong -- each hit could miss independently and had different variance.

2. **Weapon skills cannot crit in pre-renewal (systemic).** iRO Wiki Classic Stats page: "Offensive Skills do not take CRIT into account except for a few exceptions" (Sharp Shooting, etc.). Fix: skip crit roll when `isSkill === true && !forceCrit` in `calculatePhysicalDamage()`. This affects ALL weapon skills project-wide, not just Knight.

3. **Auto Counter prerequisite is 2H Sword Mastery (101), not THQ (705).** All sources (iRO Wiki Classic, RateMyServer, rAthena pre-re/skill_tree.yml) confirm "Two-Handed Sword Mastery Lv1", NOT "Two-Hand Quicken Lv1". The skill data had the wrong prerequisite (705 instead of 101).

4. **Auto Counter ignores DEF.** rAthena pre-re skill_db.yml: `DamageFlags: { IgnoreDefense: true, Critical: true }`. The counter-attack uses `ignoreDefense: true` in `calculatePhysicalDamage()`, skipping both hard DEF and soft DEF.

5. **Brandish Spear has zone-based damage multiplier (inner zones get bonus damage).** Confirmed by rAthena brandishspear.cpp source. The formula adds `baseRatio/2` at Lv4+, `baseRatio/4` at Lv7+, `baseRatio/8` at Lv10 to Zone 0 (closest targets). At Lv10, Zone 0 gets 562% vs Zone 3's base 300%.

6. **Charge Attack distance tiers are 0-3/4-6/7-9/10-12/13-14 (jRO official).** Confirmed by rAthena issue #417 with jRO official data. The previous implementation used 0-2/3-5/6-8/9-11/12+ which was wrong (off by one cell in every bracket).

7. **Charge Attack cast time scales 500/1000/1500ms.** rAthena source: `k = cap_value((distance-1)/3, 0, 2)`, casttime = `base * (1 + k)`. Base 500ms gives three tiers: 500/1000/1500. The previous implementation used a flat 1000ms.

8. **THQ provides CRI (+2+Lv) and HIT (+2*Lv) bonuses in pre-renewal.** Verified by iRO Wiki Classic THQ page, RateMyServer, Divine Pride -- ALL agree these bonuses exist in pre-renewal. The original research document incorrectly stated "Renewal additionally provides CRI and HIT bonuses -- these DO NOT exist in pre-renewal." This was wrong.

9. **Bowling Bash has chain reaction (recursive knockback + splash).** rAthena bowlingbash.cpp: secondary targets receive knockback, and at each cell along the knockback path, 3x3 splash is checked for more targets. This creates the "bowling" chain reaction. Max chain depth = skill level to prevent infinite loops.

10. **Mount speed is ~36%, not 25%.** iRO Wiki Movement Speed: base walking 0.15s/cell, mounted 0.11s/cell. Speed increase = 0.15/0.11 = 1.364 = +36.4%. The 25% value was a common private server simplification.

11. **DEF reduction during cast does NOT exist for BB or BS in pre-renewal.** rAthena source confirms this is NOT a pre-renewal mechanic. The original research document mentioned it as a feature to implement, but it was incorrect. Removed from implementation plan.

12. **Bowling Bash knockback is 1 cell fixed (not level-dependent) in pre-renewal.** Some sources reported level-based scaling (`ceil(SkillLv/2)` cells), but deep research confirmed flat 1 cell for pre-renewal. The level-based scaling was from iRO-specific patches or renewal changes.
