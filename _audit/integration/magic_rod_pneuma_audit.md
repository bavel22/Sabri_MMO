# Magic Rod Absorption & Pneuma Blocking — Cross-System Integration Audit

**Date**: 2026-03-22
**Auditor**: Claude Opus 4.6
**File**: `server/src/index.js`
**Scope**: All single-target magic skill handlers (Magic Rod) + all ranged physical skill handlers (Pneuma)

---

## Architecture Summary

### Magic Rod (`checkMagicRodAbsorption`)
- **Centralized helper**: `checkMagicRodAbsorption()` at line 1837
- Returns `true` if absorbed (caller must skip damage), `false` otherwise
- Absorbs SP based on `absorbPct` (20*lv %), broadcasts `skill:magic_rod_absorb`, removes buff after use
- **NOT integrated into `calculateMagicSkillDamage()`** — each handler must call it explicitly
- **Applies to**: Single-target magic only. AoE magic is NOT eligible per RO Classic rules.
- **Spell Breaker special case**: Magic Rod counters Spell Breaker (line 15404) — caster loses 20% MaxSP, target gains it

### Pneuma (Ground Effect)
- **Centralized in `executePhysicalSkillOnEnemy()`**: Pneuma check at line 1891-1912 when `options.isRanged === true`
- **Player auto-attack path**: Pneuma check at line 24992-24998 (bow/gun only)
- **Enemy auto-attack path**: Pneuma check at line 30620-30626 (range-based)
- **NOT integrated into `calculateSkillDamage()`** — each ranged handler must either use `executePhysicalSkillOnEnemy({ isRanged: true })` or check manually

---

## Pass 1+2: Magic Skill Handlers — Magic Rod Check Status

### Single-Target Magic Skills (Magic Rod REQUIRED)

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 1 | Cold Bolt / Fire Bolt / Lightning Bolt | 200/201/202 | 10114 | YES (line 10155) | PASS |
| 2 | Soul Strike | 210 | 10282 | YES (line 10318) | PASS |
| 3 | Frost Diver | 208 | 10907 | YES (line 10941) | PASS |
| 4 | Stone Curse | 206 | 11047 | YES (line 11079) | PASS |
| 5 | Holy Light | 414 | 12096 | YES (line 12114) | PASS |
| 6 | Jupitel Thunder | 803 | 14483 | YES (line 14496) | PASS |
| 7 | Earth Spike / Earth Spike Sage | 804/1417 | 14571 | YES (line 14584) | PASS |
| 8 | Water Ball | 806 | 14722 | YES (line 14734) | PASS |

### AoE Magic Skills (Magic Rod per-target check on player targets)

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 9 | Napalm Beat (AoE splash) | 203 | 10423 | YES per-target (line 10506) | PASS |
| 10 | Fire Ball (AoE splash) | 207 | 10573 | YES per-target (line 10613) | PASS |

### AoE Ground Magic Skills (Magic Rod NOT applicable — ground AoE)

These are ground-targeted AoE spells. Per RO Classic, Magic Rod only absorbs single-target magic, NOT ground AoE. These are correctly exempt.

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 11 | Thunderstorm | 212 | 10705 | NO (ground AoE) | CORRECT — exempt |
| 12 | Storm Gust | 803 | 14799 | NO (ground AoE) | CORRECT — exempt |
| 13 | Lord of Vermilion | 807 | 14832 | NO (ground AoE) | CORRECT — exempt |
| 14 | Meteor Storm | 808 | 14865 | NO (ground AoE) | CORRECT — exempt |
| 15 | Heaven's Drive / HD Sage | 805/1418 | 14649 | NO (ground AoE) | CORRECT — exempt |
| 16 | Fire Wall | 209 | 11184 | NO (ground AoE) | CORRECT — exempt |
| 17 | Fire Pillar | 810 | 14951 | NO (ground AoE) | CORRECT — exempt |
| 18 | Magnus Exorcismus | 1005 | 17333 | NO (ground AoE) | CORRECT — exempt |
| 19 | Sanctuary | 1003 | 17166 | NO (heal ground AoE) | CORRECT — exempt |
| 20 | Quagmire | 809 | 14902 | NO (debuff ground AoE) | CORRECT — exempt |

### Self-Centered AoE Magic (Magic Rod NOT applicable)

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 21 | Sight Rasher | 812 | 15030 | NO (self-centered AoE) | CORRECT — exempt |
| 22 | Frost Nova | 811 | 15098 | NO (self-centered AoE) | CORRECT — exempt |
| 23 | Grand Cross | 1304 | 13812 | NO (self-centered AoE, mixed phys+magic) | CORRECT — exempt |

### Reactive/Buff-Triggered Magic (Magic Rod analysis)

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 24 | Sight Blaster (reactive trigger) | 813 | 27075 | NO | **VIOLATION (L)** — see notes |
| 25 | Hindsight autocast (combat tick) | 1402 | 25378 | NO | **VIOLATION (L)** — see notes |

### Special Magic Damage Paths

| # | Skill Name | Skill ID | Line | Magic Rod Check | Status |
|---|-----------|----------|------|-----------------|--------|
| 26 | Turn Undead | 1006 | 17375 | NO | **VIOLATION (M)** — see notes |
| 27 | Monster `executeMonsterPlayerSkill` magic path | N/A | 29130 | NO | **VIOLATION (H)** — see notes |

---

## Pass 3+4: Ranged Physical Skill Handlers — Pneuma Check Status

### Skills Using `executePhysicalSkillOnEnemy` with `isRanged: true`

These correctly inherit the centralized Pneuma check:

| # | Skill Name | Skill ID | Line | `isRanged: true` | Pneuma Check | Status |
|---|-----------|----------|------|-------------------|--------------|--------|
| 1 | Spear Boomerang | 704 | 13430 | YES | INHERITED | PASS |
| 2 | Charge Attack | 710 | 13672 | YES | INHERITED | PASS |

### Skills Using `executePhysicalSkillOnEnemy` WITHOUT `isRanged`

These use the centralized function but do NOT pass `isRanged: true`, so they skip Pneuma:

| # | Skill Name | Skill ID | Line | `isRanged` passed? | Pneuma? | Status | Notes |
|---|-----------|----------|------|---------------------|---------|--------|-------|
| 3 | Musical Strike | 1536 | 18973 | NO (`{}`) | NO | **VIOLATION (M)** | Ranged attack (instrument + arrow) |
| 4 | Slinging Arrow | 1541 | 18989 | NO (`{}`) | NO | **VIOLATION (M)** | Ranged attack (whip + arrow) |
| 5 | Phantasmic Arrow | 913 | 18079 | NO (only `knockback: 3`) | NO | **VIOLATION (M)** | Ranged bow attack |
| 6 | Sand Attack | 509 | 12731 | NO | NO | CORRECT | Melee range skill |
| 7 | Mammonite | 603 | 12941 | NO | NO | CORRECT | Melee range skill |
| 8 | Shield Charge | 1305 | 13981 | NO | NO | CORRECT | Melee range skill |
| 9 | Ki Explosion | 1262 | 18504 | NO | NO | CORRECT | Melee range skill |

### Skills with Custom Damage Path (NOT using `executePhysicalSkillOnEnemy`)

| # | Skill Name | Skill ID | Line | Pneuma Check | Status | Notes |
|---|-----------|----------|------|--------------|--------|-------|
| 10 | Double Strafe | 303 | 12182 | NO | **VIOLATION (H)** | Ranged bow skill, custom damage |
| 11 | Arrow Shower | 304 | 12244 | NO | **VIOLATION (M)** | Ranged ground AoE, custom damage |
| 12 | Arrow Repel | 306 | 12358 | NO | **VIOLATION (H)** | Ranged bow skill, custom damage |
| 13 | Shield Boomerang | 1306 | 14007 | NO | **VIOLATION (H)** | Ranged thrown shield, custom damage |
| 14 | Throw Venom Knife | 1111 | 17067 | NO | **VIOLATION (M)** | Ranged thrown projectile |
| 15 | Blitz Beat | 909 | 18007 | NO | **VIOLATION (L)** | MISC damage (falcon), debatable |
| 16 | Grimtooth | 1103 | 16698 | NO | **VIOLATION (M)** | Ranged AoE from hiding |
| 17 | Acid Terror | 1801 | 20384 | YES (line 20432) | PASS | Manual check, correct |
| 18 | Double Strafe Rogue | 1706 | 19703 | NO | **VIOLATION (H)** | Ranged bow, broken call signature |
| 19 | Throw Stone | 3 | 12799 | NO | **VIOLATION (L)** | Ranged projectile, very low damage |
| 20 | Finger Offensive | 1604 | 18300 | NO | **VIOLATION (M)** | Ranged spirit spheres |

---

## Pass 5: Auto-Attack Paths

### Player Auto-Attack (PvE — line 24992)
- **Pneuma check**: YES — checks `pneuma` ground effect, blocks for `bow`/`gun` weapon types only
- **Magic Rod check**: N/A (auto-attacks are physical, not magic)
- **Status**: PASS

### Enemy Auto-Attack (line 30620)
- **Pneuma check**: YES — checks `pneuma` ground effect, blocks for ranged enemies (attackRange > melee)
- **Magic Rod check**: N/A (auto-attacks are physical)
- **Status**: PASS

### `elemental_melee` NPC Skill (line 29248)
- **Pneuma check**: NO — but this is melee, so Pneuma should not apply
- **Status**: CORRECT — melee path, exempt

### `status_melee` NPC Skill (line 29287)
- **Pneuma check**: NO — but this is melee, so Pneuma should not apply
- **Status**: CORRECT — melee path, exempt

---

## Pass 6: Centralization Analysis

### Magic Rod
- `checkMagicRodAbsorption()` is a centralized helper (line 1837), but it is NOT built into the damage calculation functions
- Each magic skill handler must explicitly call it before dealing damage
- **Risk**: Any new magic skill handler that forgets to call it will bypass absorption
- **Recommendation**: Consider adding a pre-damage hook in `calculateMagicSkillDamage()` that returns an abort signal, or create a wrapper function `executeMagicSkillDamage()` that auto-checks

### Pneuma
- `executePhysicalSkillOnEnemy()` has Pneuma built in when `isRanged: true` is passed (line 1891)
- Many ranged skills use this function but fail to pass `isRanged: true`
- Many other ranged skills have custom damage paths and don't use the helper at all
- **Risk**: Very high — most ranged skills bypass Pneuma
- **Recommendation**: Either (a) add Pneuma check to `calculateSkillDamage()` via an `isRanged` option, or (b) audit all ranged skills and add manual checks. Option (a) is preferred for maintainability.

---

## VIOLATIONS LIST

### CRITICAL (0)
None.

### HIGH SEVERITY (4)

| # | Skill | Issue | Impact |
|---|-------|-------|--------|
| H1 | Double Strafe (303) | No Pneuma check — ranged bow skill bypasses Pneuma | Players in Pneuma take full damage from DS |
| H2 | Arrow Repel (306) | No Pneuma check — ranged bow skill bypasses Pneuma | Players in Pneuma take full damage + knockback |
| H3 | Shield Boomerang (1306) | No Pneuma check — ranged thrown shield bypasses Pneuma | Crusaders can hit through Pneuma |
| H4 | Double Strafe Rogue (1706) | No Pneuma check + broken call signature (passes `dsZone` as `socket` arg) | Completely broken skill + no Pneuma |

### MEDIUM SEVERITY (8)

| # | Skill | Issue | Impact |
|---|-------|-------|--------|
| M1 | Musical Strike (1536) | Missing `isRanged: true` in options | Ranged instrument attack bypasses Pneuma |
| M2 | Slinging Arrow (1541) | Missing `isRanged: true` in options | Ranged whip attack bypasses Pneuma |
| M3 | Phantasmic Arrow (913) | Missing `isRanged: true` in options | Ranged bow attack bypasses Pneuma |
| M4 | Arrow Shower (304) | No Pneuma check — ground AoE ranged | Ground-targeted AoE, each hit is ranged physical |
| M5 | Throw Venom Knife (1111) | No Pneuma check — ranged projectile | Assassin thrown knife bypasses Pneuma |
| M6 | Grimtooth (1103) | No Pneuma check — ranged AoE from hiding | Assassin ranged AoE bypasses Pneuma |
| M7 | Turn Undead (1006) | No Magic Rod check — single-target magic | Holy magic skill can't be absorbed |
| M8 | Finger Offensive (1604) | No Pneuma check — ranged spirit spheres | Monk ranged projectile bypasses Pneuma |
| M9 | Monster `executeMonsterPlayerSkill` magic path (line 29130) | No Magic Rod check for monster-cast magic | Monsters' single-target magic (bolts, Soul Strike, etc.) ignores player Magic Rod buff |

### LOW SEVERITY (3)

| # | Skill | Issue | Impact |
|---|-------|-------|--------|
| L1 | Sight Blaster reactive (line 27075) | No Magic Rod check — reactive AoE magic | Self-centered reactive, rarely hits player targets |
| L2 | Hindsight autocast (line 25378) | No Magic Rod check — autocast bolts | PvE only (hits enemies), Magic Rod is player-only buff |
| L3 | Throw Stone (3) | No Pneuma check — ranged thrown projectile | 30 fixed damage, trivial impact |
| L4 | Blitz Beat (909) | No Pneuma check — ranged MISC damage | MISC type, debatable per RO Classic rules |

---

## Pass 7: Additional Notes

### Double Strafe Rogue (1706) — BROKEN CALL SIGNATURE
Line 19711 passes arguments in the wrong order to `executePhysicalSkillOnEnemy`:
```javascript
await executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {});
```
The function signature is:
```javascript
executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options)
```
`dsZone` is being passed as the `skill` argument. This will crash or produce undefined behavior.

### Blitz Beat Pneuma Ruling (Debatable)
In RO Classic, Blitz Beat is MISC-type damage. Per rAthena source, `BF_MISC` damage is NOT blocked by Pneuma (Pneuma only blocks `BF_LONG` + `BF_WEAPON`). Blitz Beat should arguably be exempt from Pneuma. However, some implementations do block it. Marking as LOW.

### Finger Offensive Pneuma Ruling
In rAthena, Finger Offensive (Throw Spirit Sphere) is `BF_WEAPON|BF_RANGED` in pre-renewal. It SHOULD be blocked by Pneuma. Currently missing.

### Arrow Shower Pneuma Ruling
Arrow Shower is ground-targeted AoE but each hit is `BF_WEAPON|BF_RANGED` (physical ranged). Per rAthena, Pneuma blocks Arrow Shower hits on targets standing in Pneuma. This check is missing on the per-target damage loop.

### Magic Rod on Monster-Cast Spells
The `executeMonsterPlayerSkill()` function (line 29015) handles monsters casting player-class magic spells (bolt spells, Soul Strike, etc.) against players. The magic damage path (line 29130) calculates damage directly without checking if the player target has Magic Rod active. This is the most impactful Magic Rod violation since it affects PvE combat.

---

## Recommendations

### Immediate Fixes (High Priority)

1. **Double Strafe (303)**: Add Pneuma check before damage calculation (line ~12200)
2. **Arrow Repel (306)**: Add Pneuma check before damage calculation (line ~12377)
3. **Shield Boomerang (1306)**: Add Pneuma check before damage calculation (line ~14018)
4. **Double Strafe Rogue (1706)**: Fix broken call signature, then add `isRanged: true`
5. **Monster magic path**: Add `checkMagicRodAbsorption()` call in `executeMonsterPlayerSkill()` at line 29130 for single-target magic skills

### Medium Priority Fixes

6. **Musical Strike (1536)**: Add `isRanged: true` to options at line 18973
7. **Slinging Arrow (1541)**: Add `isRanged: true` to options at line 18989
8. **Phantasmic Arrow (913)**: Add `isRanged: true` to options at line 18079
9. **Throw Venom Knife (1111)**: Add Pneuma check before damage at line ~17088
10. **Grimtooth (1103)**: Add Pneuma check per-target in AoE loop at line ~17737
11. **Turn Undead (1006)**: Add Magic Rod check before damage at line ~17393
12. **Finger Offensive (1604)**: Add Pneuma check before damage at line ~18316
13. **Arrow Shower (304)**: Add per-target Pneuma check in AoE loop at line ~12258

### Architectural Recommendation

Consider creating two wrapper functions:

```javascript
// Wrapper for single-target magic damage with automatic Magic Rod check
function executeSingleTargetMagic(caster, casterId, target, targetId, isEnemy, spCost, zone, damageFn) {
    if (!isEnemy && checkMagicRodAbsorption(target, targetId, spCost, zone)) {
        return { absorbed: true };
    }
    return damageFn();
}

// Wrapper for ranged physical damage with automatic Pneuma check
function executeRangedPhysicalDamage(target, targetPos, zone, damageFn) {
    const effects = getGroundEffectsAtPosition(targetPos.x, targetPos.y, targetPos.z || 0, 100);
    if (effects.find(e => e.type === 'pneuma')) {
        return { blocked: true, reason: 'pneuma' };
    }
    return damageFn();
}
```

This would prevent future regressions when new skills are added.

---

## Summary

| Category | Total Handlers | Passing | Violations |
|----------|---------------|---------|------------|
| Magic Rod (single-target magic) | 10 | 8 | 2 (Turn Undead, Monster magic path) |
| Magic Rod (AoE per-target) | 2 | 2 | 0 |
| Magic Rod (ground AoE — exempt) | 10 | 10 | 0 |
| Magic Rod (reactive/autocast) | 2 | 0 | 2 (low severity, mostly PvE) |
| Pneuma (via executePhysicalSkillOnEnemy isRanged) | 2 | 2 | 0 |
| Pneuma (missing isRanged flag) | 3 | 0 | 3 (Musical Strike, Slinging Arrow, Phantasmic Arrow) |
| Pneuma (custom damage path) | 10 | 1 | 9 (DS, Arrow Shower, Arrow Repel, Shield Boom, TVK, Grimtooth, DS Rogue, Finger Off., Throw Stone) |
| Pneuma (auto-attack paths) | 2 | 2 | 0 |
| Pneuma (MISC type — debatable) | 1 | — | 1 (Blitz Beat, low severity) |
| **TOTAL VIOLATIONS** | | | **4 HIGH + 9 MEDIUM + 4 LOW = 17** |

**Overall Assessment**: Magic Rod coverage is strong for player-cast single-target magic (8/10 pass). The main gap is the monster magic skill path (HIGH) and Turn Undead (MEDIUM). Pneuma coverage is weak for ranged physical skills — only 3 out of 15 ranged skill handlers have proper Pneuma blocking. The centralized `executePhysicalSkillOnEnemy` approach works well when `isRanged: true` is passed, but most ranged skills either omit the flag or use custom damage paths.
