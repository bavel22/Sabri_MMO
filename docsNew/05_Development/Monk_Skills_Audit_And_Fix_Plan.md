# Monk Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Monk_Class_Research](Monk_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

## Audit Scope
All 16 Monk skills (IDs 1600-1615) compared against RO Classic pre-renewal behavior.

**Sources verified (2026-03-16)**:
- [iROWiki Classic — Occult Impaction](https://irowiki.org/classic/Occult_Impaction) (divisor /100, ATK [350..950])
- [RateMyServer — Asura Strike](https://ratemyserver.net/index.php?page=skill_db&skid=271) (pre-renewal formula, 5-min lockout)
- [RateMyServer — Throw Spirit Sphere](https://ratemyserver.net/index.php?page=skill_db&skid=267) (ATK values, 1s cast)
- [RateMyServer — Combo Finish](https://ratemyserver.net/index.php?page=skill_db&skid=273) (5x5 AoE confirmed)
- [RateMyServer — Absorb Spirit Sphere](https://ratemyserver.net/index.php?page=skill_db&skid=262) (20% on monsters, 10 SP/sphere pre-re)
- [rAthena GitHub — battle.cpp](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp) (skillratio formulas)
- [rAthena Issue #8769](https://github.com/rathena/rathena/issues/8769) (Finger Offensive cast time scales with spheres in pre-renewal)
- [iROWiki — Spiritual Sphere Absorption](https://irowiki.org/wiki/Spiritual_Sphere_Absorption) (7 SP/sphere iRO, 10% on monsters)
- [iROWiki — Raging Trifecta Blow](https://irowiki.org/wiki/Raging_Trifecta_Blow) (proc 30-lv pre-renewal confirmed)
- [iROWiki — Guillotine Fist](https://irowiki.org/wiki/Guillotine_Fist) (formula, sphere reqs, post-cast)
- [rAthena Issue #3242](https://github.com/rathena/rathena/issues/3242) (Steel Body vs Asura interaction)
- [rAthena — status_change.txt](https://github.com/rathena/rathena/blob/master/doc/status_change.txt) (SC_STEELBODY flags)

---

## Executive Summary

**ALL 16/16 Monk skills fully implemented.** 12 audit bugs fixed + 3 previously deferred skills completed + sitting system prerequisite added.

| Category | Count | Status |
|----------|-------|--------|
| CRITICAL bugs (broken functionality) | 2 | FIXED |
| HIGH priority (wrong behavior) | 6 | FIXED |
| MEDIUM priority (missing features / data) | 4 | FIXED |
| Previously deferred (L1-L3) | 3 | COMPLETE |
| Skills verified fully correct | 16 of 16 | ALL DONE |

---

## CRITICAL BUGS

### C1: Steel Body DEF/MDEF Override Non-Functional

**Severity**: CRITICAL
**Location**: `server/src/ro_damage_formulas.js` lines 708, 896

**Problem**: Steel Body sets `overrideHardDEF: 90` and `overrideHardMDEF: 90` in the buff system, and these values propagate through `getCombinedModifiers()` (index.js lines 1168-1169). However, `ro_damage_formulas.js` NEVER reads these values. Physical DEF calculation (line 708) reads `target.hardDef` directly. Magical MDEF calculation (line 896) reads `target.hardMdef` directly.

**Impact**: Steel Body's PRIMARY defensive effect is completely non-functional. The Monk takes the same damage with or without Steel Body's DEF/MDEF override. Only the ASPD penalty (-25%) and movement speed penalty (-25%) actually work — meaning the skill is currently a pure DEBUFF with zero defensive benefit.

**RO Classic behavior (verified)**: In pre-renewal, hard DEF is a percentage reduction: `damage *= (100 - hardDEF) / 100`. Steel Body sets hard DEF=90, meaning `damage *= 10/100 = 10%`. This matches iROWiki: "reduce all incoming damage to 10%". Hard MDEF=90 provides the same 90% magical damage reduction. Soft DEF (VIT-based) still applies on top of this.

**Fix**:
```javascript
// ro_damage_formulas.js — Physical DEF step (~line 708)
// BEFORE:
const rawHardDef = Math.min(99, target.hardDef || 0);
// AFTER:
const overrideDef = (target.buffMods && target.buffMods.overrideHardDEF != null) ? target.buffMods.overrideHardDEF : null;
const rawHardDef = Math.min(99, overrideDef !== null ? overrideDef : (target.hardDef || 0));

// ro_damage_formulas.js — Magical MDEF step (~line 896)
// BEFORE:
let effectiveHardMdef = Math.min(99, target.hardMdef || target.magicDefense || 0);
// AFTER:
const overrideMdef = (target.buffMods && target.buffMods.overrideHardMDEF != null) ? target.buffMods.overrideHardMDEF : null;
let effectiveHardMdef = Math.min(99, overrideMdef !== null ? overrideMdef : (target.hardMdef || target.magicDefense || 0));
```

---

### C2: Asura Strike Standalone Sphere Requirement Wrong

**Severity**: CRITICAL
**Location**: `server/src/index.js` line 13544

**Problem**: The handler checks `(player.spiritSpheres || 0) < 1` for BOTH standalone and combo Asura.

**RO Classic behavior (verified via RateMyServer)**:
- **Standalone Asura**: Requires **5 spheres** + Fury
- **Combo Asura** (after Combo Finish): Requires **1+ spheres** + Fury
- RateMyServer: "consumes 5 Spirit Spheres, except after a combo if at least 1 Spirit Sphere remains"

**Impact**: Players can cast standalone Asura Strike with just 1 sphere + Fury, instead of needing 5 spheres. Removes the resource management aspect entirely.

**Fix**:
```javascript
// BEFORE (line 13544):
if ((player.spiritSpheres || 0) < 1) {

// AFTER:
const asuraMinSpheres = isComboAsura ? 1 : 5;
if ((player.spiritSpheres || 0) < asuraMinSpheres) {
    socket.emit('skill:error', { message: isComboAsura ? 'Requires at least 1 spirit sphere' : 'Requires 5 spirit spheres' });
    return;
}
```

---

## HIGH PRIORITY BUGS

### H1: Investigate Incorrectly Includes Passive/Mastery ATK

**Severity**: HIGH
**Location**: `server/src/index.js` lines 13408-13409

**Problem**: The Investigate ATK calculation includes `invStats.passiveATK`:
```javascript
const invATK = invStats.weaponATK + (invStats.passiveATK || 0) +
    invStats.str + Math.floor(invStats.str / 10) ** 2 + ...
```

`passiveATK` includes Iron Fists mastery (+3/lv) and spirit sphere ATK (+3/sphere). In RO Classic, mastery ATK is a flat bonus added AFTER all multipliers. Including it in the base inflates damage through BOTH the skill ratio AND the DEF multiplier.

**Comparison**: The Asura Strike handler (line 13567-13569) correctly EXCLUDES passiveATK.

**RO Classic behavior (verified)**: Investigate uses WeaponATK + StatusATK only. Mastery bonuses are flat additions after multipliers.

**Note on effectValue equivalence**: iROWiki Classic shows ATK% [350, 500, 650, 800, 950] with DEF divisor /100. Our implementation uses [175, 250, 325, 400, 475] with divisor /50. These are **mathematically equivalent**: `175/100 × DEF/50 = 350/100 × DEF/100 = 3.5 × DEF/100`. The effectValue data is CORRECT.

**Fix**:
```javascript
// BEFORE:
const invATK = invStats.weaponATK + (invStats.passiveATK || 0) +
    invStats.str + Math.floor(invStats.str / 10) ** 2 + Math.floor(invStats.dex / 5) + Math.floor(invStats.luk / 3);

// AFTER:
const invStatusATK = invStats.str + Math.floor(invStats.str / 10) ** 2 + Math.floor(invStats.dex / 5) + Math.floor(invStats.luk / 3);
const invATK = (invStats.weaponATK || 0) + invStatusATK;
// Mastery ATK (passiveATK) added after all multipliers — see below
```
Then after all multipliers (after element modifier, before Lex Aeterna):
```javascript
invDamage += (invStats.passiveATK || 0); // Flat mastery addition
```

---

### H2: Ki Explosion Splash Enemies Take No Damage

**Severity**: HIGH
**Location**: `server/src/index.js` lines 13649-13668

**Problem**: The Ki Explosion AoE loop applies knockback and stun to surrounding enemies but does NOT deal any damage to them. Only the primary target (via `executePhysicalSkillOnEnemy`) takes damage.

**RO Classic behavior**: Ki Explosion deals 300% ATK damage to the primary target AND all enemies within the 3x3 AoE. Splash enemies also receive knockback + stun.

**Fix**: Add damage calculation inside the AoE splash loop (use `calculateSkillDamage` per splash enemy, apply damage, broadcast `skill:effect_damage`, check death).

---

### H3: Finger Offensive Cast Time Not Scaling With Spheres

**Severity**: HIGH
**Location**: `server/src/ro_skill_data_2nd.js` line 183, `server/src/index.js` cast time gate
**Verified by**: [rAthena Issue #8769](https://github.com/rathena/rathena/issues/8769) — confirms pre-renewal behavior has variable cast time that scales with spheres consumed. The fix in rAthena was to make RENEWAL use fixed cast time, meaning the variable behavior IS the correct pre-renewal behavior.

**Problem**: Finger Offensive has a static `castTime: 1000` (1 second) at all levels. In RO Classic pre-renewal, the cast time scales with spheres consumed.

**rAthena base CastTime**: 500ms in skill_db. Scaling formula: `(1 + spheres_consumed) × base`. With base 500ms:

| Spheres | Expected Cast Time | Current |
|---------|-------------------|---------|
| 1       | 1000ms            | 1000ms  |
| 2       | 1500ms            | 1000ms  |
| 3       | 2000ms            | 1000ms  |
| 4       | 2500ms            | 1000ms  |
| 5       | 3000ms            | 1000ms  |

**Note**: Some sources cite `(1 + spheres) × 1000ms` (2-6s). The rAthena skill_db base of 500ms suggests the lower range. Since spheres consumed = skill level in most cases, we use **per-level cast time as a practical approximation**.

**Fix (Option A — Recommended)**: Set cast time per level in skill data:
```javascript
// ro_skill_data_2nd.js, Finger Offensive (1604):
castTime: (i+2)*500  // Lv1=1000, Lv2=1500, Lv3=2000, Lv4=2500, Lv5=3000
```

---

### H4: Finger Offensive Missing Lex Aeterna

**Severity**: HIGH
**Location**: `server/src/index.js` lines 13468-13532

**Problem**: Finger Offensive's multi-hit loop does not check for or consume Lex Aeterna. All other Monk offensive skills (Investigate, Asura Strike, Combo Finish) check for Lex Aeterna.

**RO Classic behavior**: Lex Aeterna doubles the damage of the next damaging skill, consumed after the skill finishes. Should double ALL hits.

**Fix**: Add Lex Aeterna check before the hit loop:
```javascript
let foLexActive = false;
if (foEnemy.activeBuffs) {
    const lexBuff = foEnemy.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
    if (lexBuff) {
        foLexActive = true;
        removeBuff(foEnemy, 'lex_aeterna');
        broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy: true, buffName: 'lex_aeterna', reason: 'consumed' });
    }
}
// Inside hit loop, after calculateSkillDamage:
let hitDmg = foResult.isMiss ? 0 : foResult.damage;
if (foLexActive) hitDmg *= 2;
```

---

### H5: Absorb Spirit Sphere Deducts SP On Failure

**Severity**: HIGH
**Location**: `server/src/index.js` lines 13305-13315

**Problem**: The handler deducts SP cost and applies skill delays BEFORE checking if the player has spheres to absorb. If `spheresToAbsorb <= 0`, the player loses 5 SP and incurs cooldowns but gets no benefit.

**Fix**: Move the sphere count check before SP deduction:
```javascript
if (skill.name === 'absorb_spirit_sphere') {
    // If targeting a monster: separate path (see M2)
    // Self-cast: CHECK FIRST
    const spheresToAbsorb = player.spiritSpheres || 0;
    if (spheresToAbsorb <= 0) {
        socket.emit('skill:error', { message: 'No spirit spheres to absorb' });
        return;
    }
    // THEN deduct SP and apply delays
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);
    // ... rest of handler
}
```

---

### H6: Asura Strike Missing SP Cap at 6000

**Severity**: HIGH
**Location**: `server/src/index.js` line 13570

**Problem**: The Asura Strike damage formula uses all remaining SP without a cap:
```javascript
let asDamage = Math.floor(asTotalATK * (8 + Math.floor(asRemainingMP / 10))) + effectVal;
```

**rAthena source (verified)**: The formula is `skillratio += 100 * (7 + min(sstatus->sp, 6000) / 10)` — SP is capped at 6000 in the damage calculation. This prevents extreme damage with very high SP values.

**RO Classic behavior**: SP contribution to damage is capped at 6000 SP. Beyond 6000 SP, additional SP provides zero extra damage. The excess SP is still consumed.

**Impact**: If a Monk somehow has >6000 SP (via buffs, Soul Link, or future Champion class), damage would exceed intended limits.

**Fix**:
```javascript
// BEFORE:
let asDamage = Math.floor(asTotalATK * (8 + Math.floor(asRemainingMP / 10))) + effectVal;

// AFTER:
const asuraSPCap = Math.min(asRemainingMP, 6000);
let asDamage = Math.floor(asTotalATK * (8 + Math.floor(asuraSPCap / 10))) + effectVal;
```

---

## MEDIUM PRIORITY

### M1: Ki Explosion Missing After-Cast Delay

**Severity**: MEDIUM
**Location**: `server/src/ro_skill_data_2nd.js` line 194

**Problem**: Ki Explosion has `afterCastDelay: 0` in its skill data. RO Classic pre-renewal has approximately 2000ms ACD, preventing spam.

**Fix**:
```javascript
// ro_skill_data_2nd.js, Ki Explosion (1615):
afterCastDelay: 2000  // was 0
```

---

### M2: Absorb Spirit Sphere Missing Monster Targeting

**Severity**: MEDIUM
**Location**: `server/src/index.js` lines 13305-13326

**Problem**: Absorb Spirit Sphere only supports self-cast (consume own spheres for 7 SP each). In RO Classic, it can also be cast on monsters.

**Source conflict on success rate**:
- iROWiki: "10% chance to regain SP equal to twice the target's level"
- RateMyServer pre-renewal: "20% chance, SP = 2 × monster level"

**Resolution**: Use **20%** (RateMyServer is more specific about pre-renewal mechanics). Both sources agree on `SP = monster.level × 2`. Does NOT work on Boss monsters.

**Source conflict on SP per sphere (self-cast)**:
- iROWiki (iRO version): 7 SP per sphere
- RateMyServer pre-renewal: 10 SP per sphere

**Resolution**: Keep **7 SP** (matches iRO, already implemented). Note: some pre-renewal servers used 10. This is a minor server-config-level difference.

**Fix**: Add monster targeting path before self-cast logic.

---

### M3: Combo Finish Knockback Should Be Removed From Splash

**Severity**: MEDIUM
**Location**: `server/src/index.js` (Combo Finish handler)

**Problem**: Combo Finish currently knocks back splash targets 5 cells. The rAthena Renewal database for MO_COMBOFINISH shows `Knockback: 0`. Pre-renewal sources do not consistently document knockback for this skill. RateMyServer pre-renewal mentions 5x5 AoE but does not specify knockback.

**RO Classic behavior**: Combo Finish is primarily a damage AoE. Knockback is NOT a documented pre-renewal mechanic for this skill. The knockback may have been erroneously added during implementation.

**Fix**: Remove the `knockbackTarget()` calls from the Combo Finish splash loop. Primary target should also NOT be knocked back (it's a combo skill — the Monk needs the target to stay in place for potential Asura follow-up).

---

### M4: Investigate Uses Player Soft DEF Formula on Monsters

**Severity**: MEDIUM
**Location**: `server/src/index.js` line 13411

**Problem**: The Investigate handler calculates monster soft DEF using the player formula:
```javascript
const targetSoftDEF = Math.floor((invEnemy.stats.vit || 0) * 0.5) +
    Math.max(Math.floor((invEnemy.stats.vit || 0) * 0.3), Math.floor((invEnemy.stats.vit || 0) ** 2 / 150) - 1);
```

**RO Classic behavior**: For monsters, soft DEF (def2) = VIT stat directly. The complex formula `floor(VIT*0.5) + max(floor(VIT*0.3), floor(VIT^2/150)-1)` is for PLAYERS only. iROWiki Classic says the adjustment is `(EnemyDEF + EnemyVIT) / 100` — using raw VIT.

**Impact**: For low-VIT monsters (<70), this underestimates soft DEF → Investigate does less damage. For high-VIT monsters (>70), this overestimates → Investigate does more damage than intended.

**Fix**:
```javascript
// BEFORE:
const targetSoftDEF = Math.floor((invEnemy.stats.vit || 0) * 0.5) +
    Math.max(Math.floor((invEnemy.stats.vit || 0) * 0.3), Math.floor((invEnemy.stats.vit || 0) ** 2 / 150) - 1);

// AFTER (monsters use raw VIT as soft DEF):
const targetSoftDEF = invEnemy.stats.vit || 0;
```

---

## LOW PRIORITY (DEFERRED)

### L1: Spirits Recovery (ID 1606) — COMPLETE

**Severity**: LOW (was blocked by sitting system)
**Status**: COMPLETE — sitting system implemented, Spirits Recovery fully functional.

**Implementation**:
- HP regen per tick: `(MaxHP * Lv / 500) + (4 * Lv)`
- SP regen per tick: `(MaxSP * Lv / 500) + (2 * Lv)`
- Bypasses Fury `disableSPRegen` and Asura `asura_regen_lockout`
- Works at 0-89% weight, NOT at 90%+
- Tick every 10s (normal), every 20s (50-89% weight)
- Prerequisite sitting system: `player:sit`/`player:stand` events, `player.isSitting` state

---

### L2: Blade Stop (ID 1609) — COMPLETE

**Severity**: LOW (complex mechanic)
**Status**: COMPLETE — full counter stance with root lock implemented.

**Implementation**:
1. Activate: Enter `blade_stop_catching` stance for `300 + 200*lv` ms
2. If enemy melee auto-attack hits during window: both locked with `root_lock` paired buff for `10000 + 10000*lv` ms
3. Skills available during lock per level: Lv1=none, Lv2=FO, Lv3=+Investigate, Lv4=+Chain Combo (bypasses TA req), Lv5=+Asura Strike
4. Does NOT work on Boss monsters
5. Catches melee auto-attacks only (not skills or ranged)
6. Both catcher and attacker movement/attack blocked during root_lock
7. Lock breaks on buff expiry or if either party dies

---

### L3: Ki Translation (ID 1614) — COMPLETE

**Severity**: LOW (was blocked by party system)
**Status**: COMPLETE — party system now exists, sphere transfer functional.

**Implementation**:
- Transfer 1 spirit sphere from caster to target party member
- Target cannot exceed 5 spheres
- SP cost: 40, cast time: 2000ms, ACD: 1000ms
- Range: 450 UE (9 cells)
- Validates: caster has spheres, target is Monk class in same party, target not at max spheres
- `sphere:update` broadcast for both caster and target

---

## SKILL-BY-SKILL VERIFICATION

### ID 1600 — Iron Fists (Passive)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| ATK bonus | +3 per level (3-30) | `(i+1)*3` = 3-30 | CORRECT |
| Weapon check | Knuckle or bare hand | `knuckle/fist/bare_hand/!wType` | CORRECT |
| Application | Flat mastery (after multipliers) | `getPassiveSkillBonuses()` → `bonusATK` | CORRECT |
| Prerequisites | Demon Bane 10 + Divine Protection 10 | `[413:10, 401:10]` | CORRECT |

**Verdict**: PASS

---

### ID 1601 — Summon Spirit Sphere

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| Max spheres | = skill level (1-5) | `effectValue: i+1` | CORRECT |
| SP cost | 8 (all levels) | `spCost: 8` | CORRECT |
| Cast time | 1000ms | `castTime: 1000` | CORRECT |
| Duration | 600,000ms (10 min) | `duration: 600000` | CORRECT |
| Summons per cast | 1 sphere | Handler increments by 1 | CORRECT |
| Timer reset | Resets all sphere timers | `sphereExpireTime = Date.now() + 600000` | CORRECT |
| Broadcast | sphere:update to zone | `emitSphereUpdate()` | CORRECT |
| +3 ATK/sphere | Flat mastery type | In `getEffectiveStats()` | CORRECT |

**Verdict**: PASS

---

### ID 1602 — Investigate (Occult Impaction)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| effectValue | [175..475] (equiv to [350..950]/100) | `100+i*75+75` → [175..475] | CORRECT |
| SP cost | [10, 14, 17, 19, 20] | Matches | CORRECT |
| Cast time | 1000ms (not interruptible) | `castTime: 1000` | CORRECT |
| ACD | 500ms | `afterCastDelay: 500` | CORRECT |
| Sphere cost | 1 | Handler checks/decrements 1 | CORRECT |
| Element | Always Neutral | Hardcoded neutral | CORRECT |
| Hit type | Always hits (no FLEE check) | No miss logic, `isMiss: false` | CORRECT |
| DEF as multiplier | `(hardDEF + softDEF) / 50` | `totalDEF / 50` | CORRECT |
| Mastery ATK | NOT multiplied (flat addition) | **INCLUDED in base ATK** | **BUG (H1)** |
| Monster soft DEF | Raw VIT stat | **Player formula used** | **BUG (M4)** |
| Size penalty | Applies | `getSizePenalty()` | CORRECT |
| Card bonuses | Race + size multiplicative | Applied | CORRECT |
| Lex Aeterna | Doubles damage | Checked + consumed | CORRECT |
| Status break | Checks damage-breakable | `checkDamageBreakStatuses()` | CORRECT |

**Verdict**: FAIL — H1 (passiveATK multiplied), M4 (monster soft DEF formula)

---

### ID 1603 — Triple Attack (Passive / Combo Starter)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| Proc chance | 30 - lv (29% Lv1, 20% Lv10) | `30 - tripleAttackLv` | CORRECT |
| Total ATK% | 100+20*lv (120% Lv1, 300% Lv10) | `effectValue: 120+i*20` → [120..300] | CORRECT |
| Hits | 3 per proc | 3 hits at `floor(totalPct / 3)` each | CORRECT |
| Element | Weapon element | Uses weapon element | CORRECT |
| SP cost | 0 (passive) | No SP deduction | CORRECT |
| DA priority | Double Attack procs first | DA check before TA check | CORRECT |
| Combo window | Opens Chain Combo window | `comboState` set, `skill:combo_window` emitted | CORRECT |
| Window formula | `1.3 - AGI*0.004 - DEX*0.002` | Matches | CORRECT |
| Weapon req | Knuckle/bare hand | Checked in auto-attack tick | CORRECT |
| forceHit | Combo hits always connect | `forceHit: true` | CORRECT |

**Source**: [iROWiki — Raging Trifecta Blow](https://irowiki.org/wiki/Raging_Trifecta_Blow) confirms pre-renewal proc was "30% - (Skill Level)%".

**Verdict**: PASS

---

### ID 1604 — Finger Offensive (Throw Spirit Sphere)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| ATK% per sphere | [150, 200, 250, 300, 350] | `150+i*50` → [150..350] | CORRECT (see note) |
| SP cost | 10 (flat) | `spCost: 10` | CORRECT |
| Cast time | Scales with spheres (pre-renewal) | `castTime: 1000` (flat) | **BUG (H3)** |
| ACD | 500ms | `afterCastDelay: 500` | CORRECT |
| Range | 450 UE (9 cells) | `range: 450` | CORRECT |
| Spheres consumed | min(skillLevel, available) | Matches | CORRECT |
| Hits per sphere | 1 hit at full ATK% | 1 hit per sphere | CORRECT |
| Element | Weapon element | `skillElement: null` (weapon) | CORRECT |
| forceHit | Always hits | `forceHit: true` | CORRECT |
| Lex Aeterna | Doubles all hits | **NOT checked** | **BUG (H4)** |

**ATK% source note**: RateMyServer pre-renewal shows [150, 175, 200, 225, 250]. Formula-based sources (`100+50*lv`) give [150, 200, 250, 300, 350]. Our values match the formula and multiple other references. The RMS values may represent a different server configuration.

**Verdict**: FAIL — H3 (cast time), H4 (Lex Aeterna)

---

### ID 1605 — Asura Strike (Guillotine Fist)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| Formula | `(Weapon+Status ATK)*(8+SP/10)+effectVal` | Matches | CORRECT |
| effectValue | [400, 550, 700, 850, 1000] | Matches | CORRECT |
| SP cap | min(SP, 6000) in formula | **No cap** | **BUG (H6)** |
| Cast time | [4000, 3500, 3000, 2500, 2000] | `4000-i*500` | CORRECT |
| ACD | [3000, 2500, 2000, 1500, 1000] | Matches | CORRECT |
| SP cost | ALL remaining consumed | `player.mana = 0` | CORRECT |
| Element | Always Neutral | Hardcoded neutral | CORRECT |
| Hit type | Bypasses DEF, always hits | No DEF calc, no miss | CORRECT |
| Mastery ATK | Excluded | `passiveATK` NOT in formula | CORRECT |
| Fury required | critical_explosion buff | `hasBuff` check | CORRECT |
| Sphere req (standalone) | 5 spheres | **Checks `< 1`** | **BUG (C2)** |
| Sphere req (combo) | 1+ spheres | Correctly checked | CORRECT |
| Combo bypass | After Combo Finish, no cast | `isComboAsura` check | CORRECT |
| Post-cast: SP lockout | 5-min SP regen disable | 300000ms | CORRECT |
| Post-cast: HP regen | NOT blocked (SP only) | SP only | CORRECT |
| Post-cast: Fury removed | Remove critical_explosion | `removeBuff` | CORRECT |
| Card bonuses | Race + size multiplicative | Applied | CORRECT |
| Lex Aeterna | Doubles damage | Checked + consumed | CORRECT |

**Verdict**: FAIL — C2 (sphere req), H6 (SP cap)

---

### ID 1606 — Spirits Recovery (Passive)

**Verdict**: COMPLETE (L1) — sitting system implemented, regen while sitting functional

---

### ID 1607 — Absorb Spirit Sphere

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| Self-cast SP gain | 7 SP per sphere | `spheresToAbsorb * 7` | CORRECT |
| Self-cast consume | All own spheres | `spiritSpheres = 0` | CORRECT |
| Monster target | 20% chance, SP = monsterLv × 2 | **NOT implemented** | **BUG (M2)** |
| Boss immune | Cannot drain Boss monsters | N/A | N/A |
| SP cost | 5 | `spCost: 5` | CORRECT |
| Cast time | 2000ms | `castTime: 2000` | CORRECT |
| Range | 450 UE | `range: 450` | CORRECT |
| SP deduction on fail | Should NOT deduct | **Deducts before check** | **BUG (H5)** |

**Verdict**: FAIL — H5, M2

---

### ID 1608 — Dodge (Passive)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| FLEE bonus | floor(1.5*lv) = [1,3,4,6,7,9,10,12,13,15] | `Math.floor(1.5*dodgeLv)` | CORRECT |
| Application | `bonusFLEE` in passives | `getPassiveSkillBonuses()` | CORRECT |
| Prerequisites | Iron Fists 5 + SSS 5 | `[1600:5, 1601:5]` | CORRECT |

**Verdict**: PASS

---

### ID 1609 — Blade Stop

**Verdict**: COMPLETE (L2) — counter stance with root_lock implemented

---

### ID 1610 — Chain Combo

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| ATK% | [200, 250, 300, 350, 400] | `200+i*50` → [200..400] | CORRECT |
| Hits | 4 | 4 hits at `effectVal / 4` | CORRECT |
| SP cost | [11, 12, 13, 14, 15] | `11+i` → [11..15] | CORRECT |
| Cast time | 0 (instant combo) | `castTime: 0` | CORRECT |
| isCombo | true | `isCombo: true` | CORRECT |
| Combo req | After Triple Attack | `comboState.lastSkillId === 1603` | CORRECT |
| forceHit | Always hits | `forceHit: true` | CORRECT |
| Element | Weapon element | Uses weapon element | CORRECT |
| Opens next | Combo Finish (1613) | Correct | CORRECT |
| Window formula | `1.3 - AGI*0.004 - DEX*0.002` | Matches | CORRECT |

**Verdict**: PASS

---

### ID 1611 — Critical Explosion (Fury)

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| CRIT bonus | [10, 12.5, 15, 17.5, 20] | [10, 13, 15, 18, 20] (ceil) | ACCEPTABLE |
| SP cost | 15 | `spCost: 15` | CORRECT |
| Sphere cost | 5 (all consumed) | Handler checks/consumes 5 | CORRECT |
| Duration | 180,000ms (3 min) | `duration: 180000` | CORRECT |
| SP regen disable | Natural SP regen blocked | `disableSPRegen: true` | CORRECT |
| Fury flag | Enables Asura | `furyActive: true` | CORRECT |

**Verdict**: PASS (with minor rounding note)

---

### ID 1612 — Steel Body

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| DEF override | Hard DEF = 90 (90% reduction) | Set but **NOT consumed** | **BUG (C1)** |
| MDEF override | Hard MDEF = 90 | Set but **NOT consumed** | **BUG (C1)** |
| ASPD penalty | -25% | `aspdMultiplier *= 0.75` | CORRECT |
| Move speed | -25% | `moveSpeedBonus -= 25` | CORRECT |
| Block skills | All active blocked | `blockActiveSkills: true` | CORRECT |
| Items usable | Potions still work | `inventory:use` not blocked | CORRECT |
| Passives work | TA, Dodge, Iron Fists | `skill.type !== 'passive'` | CORRECT |
| SP cost | 200 | `spCost: 200` | CORRECT |
| Cast time | 5000ms | `castTime: 5000` | CORRECT |
| Sphere cost | 5 | Handler checks/consumes 5 | CORRECT |
| Duration | [30000..150000] | `30000+i*30000` | CORRECT |

**Verdict**: FAIL — C1 (DEF/MDEF override non-functional)

---

### ID 1613 — Combo Finish

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| ATK% | [300, 360, 420, 480, 540] | `300+i*60` → [300..540] | CORRECT |
| Hits | 1 | Single hit | CORRECT |
| SP cost | [11, 12, 13, 14, 15] | `11+i` → [11..15] | CORRECT |
| Sphere cost | 1 | Handler consumes 1 | CORRECT |
| isCombo | true | `isCombo: true` | CORRECT |
| Combo req | After Chain Combo | `lastSkillId === 1610` | CORRECT |
| AoE | 5x5 splash (250 UE) | Radius 250 | CORRECT |
| Splash damage | Full effectVal% | Full damage applied | CORRECT |
| Knockback | NOT documented pre-renewal | **5 cells applied** | **BUG (M3)** |
| forceHit | Always hits | `forceHit: true` | CORRECT |
| Lex Aeterna | Doubles damage | Checked + consumed | CORRECT |
| Opens Asura | If Fury + spheres >= 1 | Correct | CORRECT |
| Zone filtering | AoE filters by zone | Correct | CORRECT |

**Verdict**: FAIL — M3 (knockback should be removed)

---

### ID 1614 — Ki Translation

**Verdict**: COMPLETE (L3) — party system exists, sphere transfer functional

---

### ID 1615 — Ki Explosion

| Aspect | Expected (RO Classic) | Current | Status |
|--------|----------------------|---------|--------|
| ATK% | 300% | `effectValue: 300` | CORRECT |
| Primary target | Full damage | `executePhysicalSkillOnEnemy()` | CORRECT |
| AoE splash | 3x3 (150 UE) | `keAoeRadius = 150` | CORRECT |
| Splash damage | 300% ATK to all | **NOT dealing damage** | **BUG (H2)** |
| Knockback | 5 cells (splash only) | Applied | CORRECT |
| Stun | 70%, 2s | Applied | CORRECT |
| HP cost | 10 | `player.health -= 10` | CORRECT |
| SP cost | 20 | `spCost: 20` | CORRECT |
| ACD | ~2000ms | **`afterCastDelay: 0`** | **BUG (M1)** |
| Zone filtering | Filters by zone | Correct | CORRECT |
| Boss stun immune | statusImmune check | Correct | CORRECT |

**Verdict**: FAIL — H2 (splash damage), M1 (ACD)

---

## Integration Points Verified

| System | Check | Status |
|--------|-------|--------|
| `getPassiveSkillBonuses()` | Iron Fists + Dodge | CORRECT |
| `getEffectiveStats()` | Spirit sphere +3 ATK | CORRECT |
| `buildFullStatsPayload()` | spiritSpheres + maxSpiritSpheres | CORRECT |
| Auto-attack tick | Triple Attack proc + combo window | CORRECT |
| Buff tick | Sphere expiry (10 min) | CORRECT |
| Buff tick | Combo window expiry | CORRECT |
| `blockActiveSkills` | Steel Body blocks active skills | CORRECT |
| `disableSPRegen` | Fury + Asura lockout block SP regen | CORRECT |
| SP regen (natural 8s) | Checks `disableSPRegen` | CORRECT |
| SP regen (skill-based 10s) | Checks `disableSPRegen` | CORRECT |
| Player init (`player:join`) | spiritSpheres/comboState initialized | CORRECT |
| Skill reset | Clears spheres/comboState | CORRECT |
| `emitSphereUpdate()` | Broadcasts to zone | CORRECT |
| Cast time gate | Combo Asura bypass | CORRECT |
| Buff registry | 3 buff types defined | CORRECT |
| `getCombinedModifiers()` | Monk modifier fields propagated | CORRECT |
| Damage pipeline | overrideHardDEF consumed (C1 FIXED) | CORRECT |
| Sitting system | `player:sit`/`player:stand` events, `player.isSitting` | CORRECT |
| Spirits Recovery | Regen tick checks `isSitting` + bypasses SP blocks | CORRECT |
| Blade Stop | `blade_stop_catching` + `root_lock` paired buffs | CORRECT |
| Ki Translation | Party member targeting, sphere transfer | CORRECT |

---

## Remediation Execution Order

Execute fixes in this order to minimize interdependency and test incrementally:

### Phase 1: Critical Fixes (2 items)
1. **C1**: Fix Steel Body DEF/MDEF override in `ro_damage_formulas.js`
2. **C2**: Fix Asura Strike standalone sphere requirement to 5

### Phase 2: High Priority Fixes (6 items)
3. **H1**: Investigate — exclude passiveATK from base, add flat after multipliers
4. **H2**: Ki Explosion — add damage to splash loop
5. **H3**: Finger Offensive — cast time `(i+2)*500` per level in skill data
6. **H4**: Finger Offensive — add Lex Aeterna check before hit loop
7. **H5**: Absorb Spirit Sphere — move sphere check before SP deduction
8. **H6**: Asura Strike — add `Math.min(SP, 6000)` cap

### Phase 3: Medium Priority (4 items)
9. **M1**: Ki Explosion — add `afterCastDelay: 2000` to skill data
10. **M2**: Absorb Spirit Sphere — add monster targeting (20% chance)
11. **M3**: Combo Finish — remove knockback from splash loop
12. **M4**: Investigate — use raw VIT for monster soft DEF

### Phase 4: Previously Deferred (3 items) — ALL COMPLETE
- L1: Spirits Recovery — DONE (sitting system implemented as prerequisite)
- L2: Blade Stop — DONE (counter stance + root_lock paired buff)
- L3: Ki Translation — DONE (party system exists, sphere transfer functional)

---

## NEW IMPLEMENTATIONS (Phase 4 Completion)

### Sitting System (Prerequisite for Spirits Recovery)

New player state system enabling sit/stand behavior.

**Socket Events:**
| Event | Direction | Payload |
|-------|-----------|---------|
| `player:sit` | Client -> Server | `{}` |
| `player:stand` | Client -> Server | `{}` |
| `player:sit_state` | Server -> Zone | `{ characterId, isSitting }` |

**Mechanics:**
- `player.isSitting` boolean state
- 2x natural HP/SP regen while sitting (standard RO behavior)
- Auto-stand triggers: taking damage, using any skill, using any item, movement
- Movement and attack blocked while sitting
- Buff name: `sitting`

### Spirits Recovery (ID 1606) — Implementation

- HP per tick: `(MaxHP * Lv / 500) + (4 * Lv)`
- SP per tick: `(MaxSP * Lv / 500) + (2 * Lv)`
- **Bypasses** Fury `disableSPRegen` and Asura `asura_regen_lockout` (unique to this skill)
- Works at 0-89% weight, does NOT work at 90%+
- Tick interval: 10s (normal weight), 20s (50-89% weight)
- Only active while `player.isSitting === true`

### Ki Translation (ID 1614) — Implementation

- Transfer 1 spirit sphere from caster to target party member
- SP cost: 40, cast time: 2000ms, ACD: 1000ms
- Range: 450 UE (9 cells)
- Validation: caster has spheres, target is Monk/Champion class in same party, target not at max spheres (5)
- `sphere:update` broadcast for both caster and target after transfer

### Blade Stop (ID 1609) — Implementation

- **Phase 1 — Catching**: Activate to enter `blade_stop_catching` buff for `300 + 200*lv` ms (500-1300ms)
- **Phase 2 — Root Lock**: If enemy melee auto-attack hits during catching window, both locked with `root_lock` paired buff for `10000 + 10000*lv` ms (20-60s)
- SP cost: 10, sphere cost: 1 (consumed on activation)
- Does NOT catch Boss monster attacks
- Does NOT catch skills or ranged attacks (melee auto-attacks only)
- **Skills usable during root lock** (per Blade Stop level):
  - Lv1: None
  - Lv2: Finger Offensive (1604)
  - Lv3: + Investigate (1602)
  - Lv4: + Chain Combo (1610) — bypasses Triple Attack requirement
  - Lv5: + Asura Strike (1605) — with Fury + spheres
- Both catcher and attacker movement/attack/skills blocked during root_lock (except whitelist above)
- Lock breaks on: buff expiry, either party dies, Dispel

**New Buff Types:**
| Buff Name | Category | Applied By |
|-----------|----------|------------|
| `sitting` | state | player:sit event |
| `blade_stop_catching` | buff | Blade Stop activation |
| `root_lock` | debuff | Blade Stop catch (paired on both players) |

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/ro_damage_formulas.js` | C1: overrideHardDEF/MDEF consumption |
| `server/src/index.js` | C2: Asura sphere check `< 5` standalone |
| `server/src/index.js` | H1: Investigate exclude passiveATK |
| `server/src/index.js` | H2: Ki Explosion splash damage |
| `server/src/ro_skill_data_2nd.js` | H3: Finger Offensive castTime `(i+2)*500` |
| `server/src/index.js` | H4: Finger Offensive Lex Aeterna |
| `server/src/index.js` | H5: Absorb sphere check before SP |
| `server/src/index.js` | H6: Asura `Math.min(SP, 6000)` cap |
| `server/src/ro_skill_data_2nd.js` | M1: Ki Explosion ACD 2000 |
| `server/src/index.js` | M2: Absorb monster targeting (20%) |
| `server/src/index.js` | M3: Combo Finish remove knockback |
| `server/src/index.js` | M4: Investigate monster soft DEF = VIT |
| `server/src/index.js` | L1: Spirits Recovery regen tick + sitting system (player:sit/stand) |
| `server/src/index.js` | L2: Blade Stop catching + root_lock handlers |
| `server/src/index.js` | L3: Ki Translation sphere transfer handler |
| `server/src/ro_buff_system.js` | New buff types: sitting, blade_stop_catching, root_lock |
| `server/src/ro_skill_data_2nd.js` | Ki Translation cast/ACD data updates |

---

## Test Checklist

After all fixes, verify:

- [ ] Steel Body: Monk takes ~10% of normal physical damage (90% reduction)
- [ ] Steel Body: Monk takes ~10% of normal magical damage
- [ ] Asura Strike standalone: Fails with <5 spheres, succeeds with 5
- [ ] Asura Strike combo: Works with 1+ spheres after Combo Finish
- [ ] Asura Strike: Damage caps at 6000 SP contribution
- [ ] Investigate: Damage is correct (no passiveATK inflation)
- [ ] Investigate: Mastery ATK added as flat after multipliers
- [ ] Investigate: Monster soft DEF uses raw VIT (not player formula)
- [ ] Ki Explosion: Splash enemies take 300% ATK damage
- [ ] Ki Explosion: Splash enemies are knocked back and stunned
- [ ] Ki Explosion: Cannot spam-cast (2s ACD)
- [ ] Finger Offensive Lv5: 3000ms cast time (not 1000ms)
- [ ] Finger Offensive: Lex Aeterna doubles all hits
- [ ] Finger Offensive: Lex Aeterna consumed after skill
- [ ] Absorb Spirit Sphere: No SP cost if no spheres (self-cast)
- [ ] Absorb Spirit Sphere: 20% chance to drain SP from monster
- [ ] Absorb Spirit Sphere: Fails on Boss monsters
- [ ] Combo Finish: No knockback on splash targets
- [ ] Combo Finish: AoE damage still works correctly
- [ ] Triple Attack: Still procs correctly at 20-29% chance
- [ ] Combo chain: TA -> Chain Combo -> Combo Finish -> Asura works
- [ ] Spirit spheres: +3 ATK each, 10-min expiry, sphere:update broadcast
- [ ] Fury: CRIT bonus, SP regen blocked, enables Asura
- [ ] Sitting: player:sit/stand events toggle isSitting, 2x regen
- [ ] Sitting: auto-stand on damage/skill/item/movement
- [ ] Spirits Recovery: HP/SP regen while sitting, correct formula per level
- [ ] Spirits Recovery: bypasses Fury and Asura SP regen blocks
- [ ] Ki Translation: transfers 1 sphere to party Monk, sphere:update for both
- [ ] Ki Translation: fails if target at max spheres or not Monk
- [ ] Blade Stop: catching stance activates for correct window duration
- [ ] Blade Stop: catches enemy melee auto-attack, root_lock applied to both
- [ ] Blade Stop: Lv4 allows Chain Combo (bypasses TA requirement)
- [ ] Blade Stop: Lv5 allows Asura Strike during root lock
- [ ] Blade Stop: does NOT catch Boss attacks, skills, or ranged
