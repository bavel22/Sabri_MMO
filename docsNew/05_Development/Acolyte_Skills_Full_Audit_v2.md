# Acolyte Skills Full Audit v2 — RO Classic Pre-Renewal Compliance

**Date:** 2026-03-17
**Scope:** All 15 Acolyte skills (IDs 400-414) — every mechanic verified against rAthena source code, iRO Wiki Classic, RateMyServer, and divine-pride.net
**Status:** COMPLETE — all 8 bugs + 3 gaps fixed, 6 items deferred
**Deep Research:** 6 parallel research agents consulted rAthena C++ source (`battle.cpp`, `status.cpp`, `skill.cpp`, `pre-re/skill_db.yml`, `pre-re/status.yml`), iRO Wiki Classic, RateMyServer, divine-pride.net, and Hercules board discussions

---

## Audit Methodology

1. Read all 15 skill definitions from `ro_skill_data.js` (every field, every level)
2. Read all handler implementations from `index.js` (lines 8036-8656, every line)
3. Read all buff definitions and modifier cases from `ro_buff_system.js`
4. Read passive implementations from `getPassiveSkillBonuses()` (lines 485-584)
5. Verified damage pipeline integration (`ro_damage_formulas.js` lines 700-750)
6. Verified Ruwach detection tick (lines 19105-19168)
7. Verified Pneuma ranged block (lines 17659-17665, 20741-20744)
8. Verified Warp Portal collision/teleport/removal (lines 4467-4512)
9. Verified `calculateHealAmount()` formula (lines 1213-1224)
10. Deep research: fetched rAthena source code from GitHub, cross-referenced `#ifdef RENEWAL` guards, verified pre-renewal code paths
11. Resolved all source conflicts using rAthena as authoritative (closest to official server behavior)

---

## SKILL-BY-SKILL AUDIT RESULTS

### Heal (400) — 1 BUG

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Formula | `floor((BaseLv+INT)/8) * (4+8*SkillLv)` | `calculateHealAmount()` — same formula | CORRECT |
| MATK in formula | NO (pre-renewal uses BaseLv+INT only) | Not used | CORRECT |
| SP Cost | 13,16,19,22,25,28,31,34,37,40 (`10+3*lv`) | Same | CORRECT |
| Cast Time | 0 | 0 | CORRECT |
| After-Cast Delay | 1000ms | 1000ms | CORRECT |
| **Undead Damage** | **50% of heal amount * element modifier** | **100% of heal amount** | **BUG** |
| MDEF Ignore | Heal damage ignores MDEF | No MDEF reduction applied | CORRECT |
| Undead Check | Undead PROPERTY only (not race) | `enemyEle !== 'undead'` (property check) | CORRECT |
| Self/Ally Heal | Caps at maxHealth | `Math.min(maxHealth, health + healAmount)` | CORRECT |
| Heal Power Mods | `equipHealPower` + `cardHealPower` | Both applied in `calculateHealAmount()` | CORRECT |
| Auto Berserk | Deactivate above 25% HP | `checkAutoBerserk()` called after heal | CORRECT |
| Range | 450 UE (9 cells) | 450 | CORRECT |
| Ally Targeting | Can heal other players | `connectedPlayers.get(targetId)` | CORRECT |

#### BUG: Undead damage should be 50%, not 100%

The code at line 8060 has a comment: "RO Classic pre-renewal: Heal deals FULL heal amount as Holy damage to Undead (not 50% — that was a Renewal change)." **This comment is incorrect.**

**rAthena source code** (`skill_calc_heal` in `skill.cpp`): The halving is in the **common code path** (NOT inside `#ifdef RENEWAL`):
```cpp
if( (!heal || (target && target->type == BL_MER)) && skill_id != NPC_EVILLAND )
    hp /= 2;
```
When `castendDamageId` routes through `skill_attack(BF_MAGIC, ...)`, it calls `skill_calc_heal(..., heal=false)`, triggering `/= 2`.

**All 4 sources agree on 50%:**
- iRO Wiki Classic: "Heal does holy damage half as the normal healing"
- RateMyServer: "deals only half damage"
- divine-pride.net: "Holy property damage equal to half the amount of the HP restored"
- rAthena: `hp /= 2` in common (non-RENEWAL) code path

**Fix** (line 8063):
```javascript
// BEFORE:
const holyDamage = Math.max(1, Math.floor(healAmount * holyVsUndead / 100));

// AFTER (rAthena: hp /= 2 in common code path):
const holyDamage = Math.max(1, Math.floor(Math.floor(healAmount / 2) * holyVsUndead / 100));
```
Also fix the comment at line 8060.

---

### Divine Protection (401) — BUG: Missing Base Level Scaling

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Type | Passive | Passive | CORRECT |
| vs Undead element / Demon race | Flat soft DEF addition | `raceDEF.undead/demon` | CORRECT |
| Monsters only | Does NOT work vs players | Race-based (PvE only) | CORRECT |
| **Formula (rAthena)** | **`floor((BaseLv/25 + 3) * SkillLv + 0.5)`** | **`dpLv * 3` (flat)** | **BUG** |

**rAthena source** (`battle.cpp` line 4928-4930):
```cpp
vit_def += (int32)(((float)tsd->status.base_level / 25.0 + 3.0) * skill + 0.5);
```

**Fix** (line 544-546):
```javascript
// BEFORE:
bonuses.raceDEF.undead = (bonuses.raceDEF.undead || 0) + dpLv * 3;
bonuses.raceDEF.demon = (bonuses.raceDEF.demon || 0) + dpLv * 3;

// AFTER (rAthena battle.cpp authoritative):
const dpBaseLv = player.stats.level || player.stats.base_level || 1;
const dpBonus = Math.floor((dpBaseLv / 25 + 3) * dpLv + 0.5);
bonuses.raceDEF.undead = (bonuses.raceDEF.undead || 0) + dpBonus;
bonuses.raceDEF.demon = (bonuses.raceDEF.demon || 0) + dpBonus;
```

| BaseLv | Lv5 Total | Lv10 Total | Current Lv10 |
|--------|----------|------------|--------------|
| 1 | 15 | 30 | 30 |
| 50 | 25 | 50 | 30 |
| 75 | 30 | 60 | 30 |
| 99 | 35 | 70 | 30 |

---

### Blessing (402) — CORRECT (all verified)

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| PATH 1: Enemy Debuff | Undead element OR Demon race (rAthena: `battle_check_undead \|\| RC_DEMON`) | Both checked | CORRECT |
| Debuff Effect | Halve STR, DEX, AND INT (all three — rAthena source explicit) | `Math.floor(stat / 2)` for all three | CORRECT |
| Boss Immunity | `BossResist: true` in rAthena `pre-re/status.yml` | Checked before SP | CORRECT |
| Cannot debuff players | `bl->type == BL_PC` always gets buff path | No player debuff path | CORRECT |
| PATH 2: Cure | Cures Curse AND Stone (Curse checked first) | `cleanseStatusEffects(['curse', 'stone'])` | CORRECT |
| No buff on cure | `return true` after status removal | Returns after cure | CORRECT |
| PATH 3: Buff | +SkillLv to STR/DEX/INT | `effectVal` = 1-10 | CORRECT |
| Duration | `40 + 20*lv` (60-240s) | Skill data matches | CORRECT |
| SP Cost | `24 + 4*lv` (28-64) | Skill data matches | CORRECT |
| Cast Time | 0 (instant) | 0 | CORRECT |
| Blessing ≠ cancel IncAGI | No `EndOnStart` entry (rAthena `pre-re/status.yml`) | No removal code | CORRECT |

---

### Increase AGI (403) — 1 BUG

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| HP Cost | 15 HP, fail if < 16 HP | `player.health < 16` check, `-15` deduction | CORRECT |
| AGI Bonus | `2+SkillLv` (3-12) (rAthena: `val2 = 2 + val1`) | `effectVal` = 3-12 | CORRECT |
| Move Speed | +25% (rAthena: `val = max(val, 25)`) | `moveSpeedBonus: 25` | CORRECT |
| SP Cost | `15+3*lv` (18-45) | Skill data matches | CORRECT |
| Cast Time | 1000ms | 1000ms | CORRECT |
| ACD | 1000ms | 1000ms | CORRECT |
| Duration | `40+20*lv` (60-240s) | Skill data matches | CORRECT |
| Cancels DecAGI | `EndOnStart: Decreaseagi` | `removeBuff(target, 'decrease_agi')` | CORRECT |
| Stats Broadcast | Target gets AGI/ASPD update | `player:stats` sent to target | CORRECT |
| **Fails if Quagmire** | **Fail list in rAthena: will not apply if Quagmire active** | **No check** | **BUG** |

**Fix** (before `applyBuff` in increase_agi handler, ~line 8267):
```javascript
// Increase AGI fails if target has Quagmire (rAthena: Fail list)
if (hasBuff(buffTarget, 'quagmire')) {
    socket.emit('skill:error', { message: 'Cannot apply — Quagmire active' });
    // SP still consumed (already deducted above)
    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

---

### Decrease AGI (404) — 3 BUGS

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Success Rate | `40+2*lv+floor((BaseLv+INT)/5)-MDEF` | Same formula | CORRECT |
| Clamp 5-95% | Yes | Yes | CORRECT |
| SP on Failure | Consumed | SP deducted before roll | CORRECT |
| Boss Immunity | `BossResist: true` | Checked before SP | CORRECT |
| AGI Reduction | `2+SkillLv` (3-12) | `effectVal` = 3-12 | CORRECT |
| Move Speed | -25% | `moveSpeedReduction: 25` | CORRECT |
| **Monster Duration** | **`30+10*lv` (40-130s) (rAthena `Duration1`)** | **`20+10*lv` (30-120s)** | **BUG** |
| Player Duration | Monster/2 (20-65s) | `(3+lv)*5000` (20-65s) | CORRECT |
| Cancels IncAGI | Yes | Yes | CORRECT |
| **Cancels speed buffs** | **Also removes adrenaline, adrenaline2, twohand_quicken, one_hand_quicken, spear_quicken, cart_boost** | **Only removes increase_agi** | **BUG** |
| **Player Stats Update** | **ASPD recalculated via `status_calc_bl`** | **No player:stats broadcast** | **BUG** |

#### Bug 1: Monster duration data is wrong in `ro_skill_data.js`

rAthena `pre-re/skill_db.yml` Duration1 for Decrease AGI: Lv1=40s, Lv2=50s... Lv10=130s (`30 + 10*lv`).
Current skill data: Lv1=30s, Lv2=40s... Lv10=120s (`20 + 10*lv`). Off by 10s at every level.

**Fix** in `ro_skill_data.js`:
```
Lv1:  duration: 30000 → 40000
Lv2:  duration: 40000 → 50000
Lv3:  duration: 50000 → 60000
Lv4:  duration: 60000 → 70000
Lv5:  duration: 70000 → 80000
Lv6:  duration: 80000 → 90000
Lv7:  duration: 90000 → 100000
Lv8:  duration: 100000 → 110000
Lv9:  duration: 110000 → 120000
Lv10: duration: 120000 → 130000
```

#### Bug 2: Missing speed buff dispels

rAthena `pre-re/status.yml` `EndOnStart` for SC_DECREASEAGI:
```
Increaseagi, Adrenaline, Adrenaline2, Twohandquicken, Onehand, Spearquicken, Cartboost, Merc_Quicken
```

**Fix** (after existing `removeBuff(target, 'increase_agi')`, ~line 8349):
```javascript
// Decrease AGI also removes speed-related buffs (rAthena EndOnStart list)
const speedBuffsToRemove = [
    'adrenaline_rush', 'full_adrenaline_rush',
    'twohand_quicken', 'one_hand_quicken', 'spear_quicken', 'cart_boost'
];
for (const speedBuff of speedBuffsToRemove) {
    if (hasBuff(target, speedBuff)) {
        removeBuff(target, speedBuff);
        broadcastToZone(player.zone || 'prontera_south', 'skill:buff_removed', {
            targetId, isEnemy, buffName: speedBuff, reason: 'decrease_agi_dispel'
        });
    }
}
```

#### Bug 3: Missing player:stats broadcast

**Fix** (after `applyBuff`, ~line 8359, for player targets only):
```javascript
if (!isEnemy) {
    const daEffStats = getEffectiveStats(target);
    const daNewDerived = roDerivedStats(daEffStats);
    const daSock = io.sockets.sockets.get(target.socketId);
    if (daSock) {
        daSock.emit('player:stats', buildFullStatsPayload(
            targetId, target, daEffStats, daNewDerived,
            Math.min(COMBAT.ASPD_CAP, daNewDerived.aspd + (target.weaponAspdMod || 0))
        ));
        daSock.emit('combat:health_update', {
            characterId: targetId, health: target.health,
            maxHealth: target.maxHealth, mana: target.mana, maxMana: target.maxMana
        });
    }
}
```

---

### Cure (405) — 1 BUG, 1 GAP

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Cures | Silence, Blind, Confusion (NOT Hallucination) | `['silence', 'blind', 'confusion']` | CORRECT |
| SP Cost | 15 | 15 | CORRECT |
| Cast Time | 0 | 0 | CORRECT |
| **ACD** | **1000ms (rAthena pre-re skill_db)** | **0** | **BUG** |
| Range | 450 UE | 450 | CORRECT |
| Target | Self/Ally | Self + allies | CORRECT |
| **Offensive vs Undead** | **Inflicts Confusion on Undead targets** | **Not implemented** | **GAP** |

#### Bug: Missing after-cast delay

rAthena `pre-re/skill_db.yml` for AL_CURE has `AfterCastActDelay: 1000`.

**Fix** in `ro_skill_data.js`, cure levels:
```javascript
afterCastDelay: 0  →  afterCastDelay: 1000
```

#### Gap: Offensive use on Undead monsters

rAthena source has a `battle_check_undead()` branch in the AL_CURE case that inflicts SC_CONFUSION on Undead targets. This is a niche mechanic.

**Fix** (add offensive path to cure handler, before the cleanse logic):
```javascript
// Cure cast on Undead element/race enemy → inflicts Confusion
if (targetId && isEnemy) {
    const enemy = enemies.get(targetId);
    if (enemy && !enemy.isDead) {
        const enemyEle = (enemy.element && enemy.element.type) || 'neutral';
        const enemyRace = enemy.race || 'formless';
        if (enemyEle === 'undead' || enemyRace === 'undead') {
            // Apply confusion status
            applyStatusEffect(enemy, 'confusion', 10000, characterId);
            broadcastToZone(player.zone, 'status:applied', {
                targetId, isEnemy: true, statusType: 'confusion', duration: 10000
            });
        }
    }
}
```

---

### Angelus (406) — CORRECT (all verified)

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Bonus | +5*SkillLv % to Soft DEF (DEF2) | `defPercent: effectVal` (5-50) | CORRECT |
| rAthena CalcFlag | `Def2: true` (NOT `Def`) | Applied to `effectiveSoftDef` | CORRECT |
| Pipeline | `def2 += def2 * val2 / 100` | `angelusMultiplier = 1 + defPercent/100` | CORRECT |
| Duration | `30*SkillLv` (30-300s) | Skill data matches | CORRECT |
| SP Cost | `20+3*lv` (23-50) | Skill data matches | CORRECT |
| Cast Time | 500ms | 500ms | CORRECT |
| ACD | 3500ms | 3500ms | CORRECT |
| Party-wide | `SplashArea: -1` (screen-wide party) | Self-only | DEFERRED |

---

### Signum Crucis (407) — 1 BUG

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| **Success Rate** | **`25+4*lv+BaseLv-TargetLv` (rAthena commit 5e63d8b)** | **`23+4*lv+BaseLv-TargetLv`** | **BUG** |
| DEF Reduction | `10+4*lv` % (14-50%) of Hard DEF | `effectVal` = 14-50, `defMultiplier` | CORRECT |
| rAthena CalcFlag | `Def: true` (hard DEF, NOT soft DEF) | Applied to `hardDef` via `defMultiplier` (line 744) | CORRECT |
| Duration | Permanent (no Duration1 in skill_db) | 86400000ms | CORRECT |
| Target Filter | Undead element OR Demon race | Both checked | CORRECT |
| AoE Radius | `SplashArea: 15` (~31x31 cells) | 750 UE (~15 cells each side) | CORRECT |
| Works on Bosses | Yes (confirmed iRO Wiki Classic) | No boss check | CORRECT |
| Does NOT work on players | Even with Undead armor | No player targeting | CORRECT |

#### Bug: Success rate base constant is 25, not 23

rAthena official commit `5e63d8b` corrected the Signum Crucis success rate formula base constant from 23 to **25**. The old value of 23 matches some outdated iRO Wiki tables (27% at Lv1), but the corrected rAthena value is authoritative.

**Fix** (line 8423):
```javascript
// BEFORE:
const scSuccess = Math.min(95, Math.max(5, 23 + 4 * learnedLevel + casterBaseLv - targetLv));

// AFTER (rAthena commit 5e63d8b):
const scSuccess = Math.min(95, Math.max(5, 25 + 4 * learnedLevel + casterBaseLv - targetLv));
```

---

### Ruwach (408) — CORRECT

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Duration | 10s | 10000ms | CORRECT |
| SP Cost | 10 | 10 | CORRECT |
| Cast Time | 0 | 0 | CORRECT |
| Detection Tick | Periodic scan around caster | 1s tick in regen loop (line 19105) | CORRECT |
| Reveal Radius | 5x5 cells (250 UE) | 250 UE | CORRECT |
| Damage on Reveal | 145% MATK Holy | `calculateMagicSkillDamage(..., 145, 'holy')` | CORRECT |
| Removes | Hiding, Cloaking | Both checked | CORRECT |
| Follows Caster | SC_RUWACH with OPTION flag | Buff on player, position from `player.lastX/Y` | CORRECT |
| Hidden Enemies | Should detect hidden monsters | Not checked (no enemy hiding system yet) | DEFERRED |

---

### Teleport (409) — DEFERRED GAP

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| SP Cost | Lv1=10, Lv2=9 | 10, 9 | CORRECT |
| Lv1 | Random warp on same map | Random from spawn ±1000 UE | CORRECT |
| Lv2 | Choice menu: Random OR Save Point | Always save point | DEFERRED |
| Cast Time | 0 | 0 | CORRECT |

---

### Warp Portal (410) — 2 GAPS

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Blue Gemstone | Consumed (item 717) | DB UPDATE + rowCount | CORRECT |
| Gem Bypass | `cardNoGemStone` (Mistress) | Checked | CORRECT |
| SP Cost | 35,32,29,26 | Matches | CORRECT |
| Cast Time | 1000ms | 1000ms | CORRECT |
| Portal Duration | 10,15,20,25s | Matches | CORRECT |
| Collision | Teleport on walk-in | Line 4470 | CORRECT |
| Remove on Use | `removeGroundEffect(portal.id)` | Line 4507 | CORRECT |
| **Max Portals** | **3 per caster** | **No limit** | **GAP** |
| **Max Users** | **8 per portal** | **Removed after 1st use** | **GAP** |
| Memorize | Up to 3 memo destinations | DEFERRED | DEFERRED |

**Gap fixes:** Same as original plan (see Execution Plan below).

---

### Pneuma (411) — CORRECT

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Duration | 10s | 10000ms | CORRECT |
| SP Cost | 10 | 10 | CORRECT |
| Blocks | ALL ranged physical (4+ cell range) | `attackRange > MELEE_RANGE + RANGE_TOLERANCE` | CORRECT |
| Does NOT block magic | Correct | No Pneuma check in magic path | CORRECT |
| Enemy→Player block | Auto-attack tick | Line 17659-17665 | CORRECT |
| Enemy attack on player block | Enemy attack tick | Line 20741-20744 | CORRECT |
| Overlap prevention | Cannot overlap Pneuma/Safety Wall | `getGroundEffectsAtPosition()` check | CORRECT |
| 3x3 ground cell | 100 UE radius | Matches | CORRECT |

---

### Aqua Benedicta (412) — STUB (Known Deferred)

All deferred (water cell, Empty Bottle, Holy Water creation). SP/cast time correct.

---

### Demon Bane (413) — BUG: Missing Base Level Scaling

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Type | Passive | Passive | CORRECT |
| vs Undead element / Demon race | Flat ATK bonus (mastery type) | `raceATK.undead/demon` | CORRECT |
| Monsters only | Does NOT work vs players | Race-based (PvE only) | CORRECT |
| **Formula (rAthena)** | **`floor(SkillLv * (BaseLv / 20 + 3))`** | **`dbLv * 3` (flat)** | **BUG** |

**rAthena source** (`battle.cpp` line 2283-2286):
```cpp
damage += static_cast<decltype(damage)>(skill * (sd->status.base_level / 20.0 + 3.0));
```
Note: No intermediate floor — the float result is truncated to int at the END.

**Fix** (line 551-553):
```javascript
// BEFORE:
bonuses.raceATK.undead = (bonuses.raceATK.undead || 0) + dbLv * 3;
bonuses.raceATK.demon = (bonuses.raceATK.demon || 0) + dbLv * 3;

// AFTER (rAthena battle.cpp authoritative):
const dbBaseLv = player.stats.level || player.stats.base_level || 1;
const dbBonus = Math.floor(dbLv * (dbBaseLv / 20 + 3));
bonuses.raceATK.undead = (bonuses.raceATK.undead || 0) + dbBonus;
bonuses.raceATK.demon = (bonuses.raceATK.demon || 0) + dbBonus;
```

| BaseLv | Lv5 Total | Lv10 Total | Current Lv10 |
|--------|----------|------------|--------------|
| 1 | 15 | 30 | 30 |
| 50 | 27 | 55 | 30 |
| 75 | 33 | 67 | 30 |
| 99 | 39 | **79** | 30 |

Note: Demon Bane uses `/20` (not `/25` like Divine Protection), and does NOT round to nearest (`+0.5`). Different formulas.

---

### Holy Light (414) — CORRECT

| Property | Expected (RO Classic) | Current | Status |
|----------|----------------------|---------|--------|
| Damage | 125% MATK | `calculateMagicSkillDamage(..., 125, 'holy')` | CORRECT |
| Element | Holy | `'holy'` | CORRECT |
| SP Cost | 15 | 15 | CORRECT |
| Cast Time | 2000ms | 2000ms | CORRECT |
| Status Break | freeze/stone/sleep/confusion | `checkDamageBreakStatuses()` | CORRECT |
| Kyrie Removal | Cancels Kyrie on hit (confirmed by all sources) | DEFERRED | DEFERRED |
| Priest Spirit 5x | 625% MATK, 75 SP | DEFERRED | DEFERRED |

---

## SUMMARY OF ALL ISSUES

### BUGS (8 issues — must fix)

| # | Skill | Issue | Severity | Source |
|---|-------|-------|----------|--------|
| B1 | Heal (400) | Undead damage = 50%, not 100% | **HIGH** | rAthena `skill_calc_heal`: `hp /= 2` in common path |
| B2 | Divine Protection (401) | Missing BaseLv scaling | **HIGH** | rAthena `battle.cpp`: `(BaseLv/25 + 3) * SkillLv` |
| B3 | Demon Bane (413) | Missing BaseLv scaling | **HIGH** | rAthena `battle.cpp`: `SkillLv * (BaseLv/20 + 3)` |
| B4 | Decrease AGI (404) | Missing speed buff dispels (7 buffs) | **MEDIUM** | rAthena `pre-re/status.yml` EndOnStart |
| B5 | Decrease AGI (404) | No player:stats broadcast after debuff | **MEDIUM** | rAthena `status_calc_bl` triggers recalc |
| B6 | Decrease AGI (404) | Monster duration 10s too short at every level | **MEDIUM** | rAthena Duration1: `30+10*lv` vs our `20+10*lv` |
| B7 | Signum Crucis (407) | Success rate base = 25, not 23 | **LOW** | rAthena commit `5e63d8b` |
| B8 | Cure (405) | Missing 1000ms after-cast delay | **LOW** | rAthena `pre-re/skill_db.yml` |

### GAPS (3 issues — should fix)

| # | Skill | Issue | Severity |
|---|-------|-------|----------|
| G1 | Cure (405) | Offensive Confusion on Undead monsters | LOW |
| G2 | Warp Portal (410) | No max portal count (RO: 3 per caster) | LOW |
| G3 | Warp Portal (410) | Single-use instead of 8-use | LOW |

### Increase AGI Quagmire Fail Check (B9 — conditional)

| # | Skill | Issue | Severity | Note |
|---|-------|-------|----------|------|
| B9 | Increase AGI (403) | No Quagmire fail check | **LOW** | Only matters when Quagmire is active (Wizard/Sage) |

### DEFERRED (6 items — acceptable, blocked by other systems)

| # | Skill | Feature | Blocked By |
|---|-------|---------|-----------|
| D1 | Angelus (406) | Party-wide application | Party system |
| D2 | Warp Portal (410) | Memorize system (/memo) | Memo DB + destination UI |
| D3 | Aqua Benedicta (412) | Full implementation | Item creation + water cell |
| D4 | Holy Light (414) | Kyrie Eleison removal | Kyrie interaction hook |
| D5 | Teleport (409) | Lv2 choice menu | Skill selection UI |
| D6 | Ruwach (408) | Detect hidden enemies | Enemy hiding system |

---

## EXECUTION PLAN

### Phase 1: Critical Bug Fixes (B1-B3) — `index.js`

1. **B1: Heal undead damage 50%** (line 8063)
   - Change: `healAmount * holyVsUndead / 100` → `Math.floor(healAmount / 2) * holyVsUndead / 100`
   - Fix comment at line 8060

2. **B2: Divine Protection BaseLv scaling** (line 544-546)
   - Change: `dpLv * 3` → `Math.floor((dpBaseLv / 25 + 3) * dpLv + 0.5)`

3. **B3: Demon Bane BaseLv scaling** (line 551-553)
   - Change: `dbLv * 3` → `Math.floor(dbLv * (dbBaseLv / 20 + 3))`

### Phase 2: Decrease AGI Fixes (B4-B6) — `index.js` + `ro_skill_data.js`

4. **B6: Fix monster duration data** in `ro_skill_data.js`
   - All 10 levels: add 10000ms to each duration value

5. **B4: Add speed buff dispels** (after line 8349)
   - Remove: `adrenaline_rush`, `full_adrenaline_rush`, `twohand_quicken`, `one_hand_quicken`, `spear_quicken`, `cart_boost`

6. **B5: Add player:stats broadcast** (after line 8359)
   - Mirror the pattern from Increase AGI handler

### Phase 3: Minor Fixes (B7-B9) — `index.js` + `ro_skill_data.js`

7. **B7: Signum Crucis base constant** (line 8423)
   - Change: `23 +` → `25 +`

8. **B8: Cure after-cast delay** in `ro_skill_data.js`
   - Change: `afterCastDelay: 0` → `afterCastDelay: 1000`

9. **B9: Increase AGI Quagmire fail check** (~line 8267)
   - Add: `if (hasBuff(buffTarget, 'quagmire'))` → error + return

### Phase 4: Gap Fixes (G1-G3) — `index.js`

10. **G1: Cure offensive Confusion on Undead** — add enemy path to cure handler
11. **G2: Max 3 portals per caster** — count + remove oldest
12. **G3: 8-use portals** — add `usesRemaining` counter

---

## TESTING CHECKLIST

### Phase 1
- [ ] B1: Heal on Undead Lv1 enemy at BaseLv50 INT50 SkillLv10 → damage = `floor(floor(1050/2) * 100/100)` = 525 (was 1050)
- [ ] B1: Heal on ally → same heal amount as before (50% only applies to undead)
- [ ] B2: Divine Protection Lv10 BaseLv99 → 70 DEF reduction (was 30)
- [ ] B2: Divine Protection Lv10 BaseLv1 → 30 DEF reduction (unchanged at low level)
- [ ] B3: Demon Bane Lv10 BaseLv99 → 79 ATK bonus (was 30)
- [ ] B3: Demon Bane Lv10 BaseLv1 → 30 ATK bonus (unchanged at low level)

### Phase 2
- [ ] B6: Decrease AGI Lv1 on monster → 40s duration (was 30s)
- [ ] B6: Decrease AGI Lv10 on monster → 130s duration (was 120s)
- [ ] B4: DecAGI on target with Adrenaline Rush → buff removed
- [ ] B4: DecAGI on target with Two-Hand Quicken → buff removed
- [ ] B4: DecAGI on target with Spear Quicken → buff removed
- [ ] B5: DecAGI on player target → client ASPD updates immediately

### Phase 3
- [ ] B7: Signum Crucis Lv1 same-level target → 29% success (was 27%)
- [ ] B8: Cure → 1s delay before next skill use
- [ ] B9: Increase AGI on Quagmire target → fails with error

### Phase 4
- [ ] G1: Cure on Undead monster → Confusion applied
- [ ] G2: 4th Warp Portal cast → oldest removed
- [ ] G3: 8 players enter same portal → all teleport, portal removed after 8th

### Regressions
- [ ] Heal still heals allies correctly
- [ ] Blessing 3-path still works (debuff/cure/buff)
- [ ] Increase AGI still removes Decrease AGI
- [ ] Decrease AGI still removes Increase AGI
- [ ] Pneuma still blocks ranged attacks
- [ ] Warp Portal gem consumption still works
- [ ] Holy Light still deals 125% MATK Holy damage
