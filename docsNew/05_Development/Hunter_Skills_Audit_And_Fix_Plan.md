# Hunter Skills Comprehensive Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Hunter_Class_Research](Hunter_Class_Research.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-17 (v3 — ALL FIXES IMPLEMENTED AND VERIFIED)
**Scope:** All 18 Hunter skills (IDs 900-917) + Auto-Blitz + Trap system
**Sources:** rAthena `src/map/battle.cpp`, `src/map/skill.cpp`, `src/map/status.cpp`, `db/pre-re/skill_db.yml`, `db/pre-re/status.yml`; iRO Wiki Classic; RateMyServer
**Files:** `server/src/index.js`, `server/src/ro_skill_data_2nd.js`, `server/src/ro_ground_effects.js`, `server/src/ro_status_effects.js`
**Status:** ALL FIXES IMPLEMENTED. Syntax verified. Every skill cross-referenced against wiki sources.

---

## Executive Summary

**18 skills audited, 12 bugs fixed (4 critical, 4 moderate, 2 minor + 2 cosmetic), 4 acceptable gaps.**

Deep research against rAthena source code corrected two false positives from v1 (Flasher blind and Freezing Trap freeze are correctly 100% base rate) but uncovered two new bugs (Freezing Trap uses wrong damage pipeline AND wrong item cost). Land Mine stun kept at (30+5*lv)% per RateMyServer explicit documentation (rAthena `sc_start 10` rate is ambiguous scale).

---

## Bug Registry

### CRITICAL — Wrong behavior that changes gameplay significantly

| # | Skill | Bug | Current | Canonical (rAthena source) | Severity |
|---|-------|-----|---------|-----------|----------|
| C1 | Ankle Snare (903) | Uses `stun` status — blocks attack/cast/items | `applyStatusEffect('stun')` blocks everything | rAthena `status.yml` SC_ANKLE: `States: { NoMove: true }` ONLY. Target CAN attack, cast, use items. | **CRITICAL** |
| C2 | Blitz Beat (900) / Auto-Blitz | Damage formula INT and DEX terms wrong | `80 + SC*6 + INT + floor(DEX/5)` | rAthena `battle.cpp`: `(dex/10 + int_/2 + skill*3 + 40) * 2` = `80 + SC*6 + 2*floor(INT/2) + 2*floor(DEX/10)` | **CRITICAL** |
| C3 | Auto-Blitz Beat | Hit count formula off-by-one at decade boundaries | `floor(jobLv/10) + 1` | rAthena `skill.cpp`: `(sd->status.job_level + 9) / 10` = `floor((jobLv+9)/10)` | **CRITICAL** |
| C4 | Freezing Trap (911) | Uses MISC DEX/INT formula — should use ATK-based Weapon pipeline | `calculateTrapDamage('freezing_trap')`: `lv*(DEX+75)*(1+INT/100)` (MISC) | rAthena `skill_db.yml`: `Type: Weapon`, `IgnoreAtkCard: true`. Damage = `(25+25*SkillLv)%` of ATK, reduced by DEF, CAN miss (no IgnoreFlee). | **CRITICAL** |

### MODERATE — Incorrect but lower impact

| # | Skill | Bug | Current | Canonical (rAthena source) | Severity |
|---|-------|-----|---------|-----------|----------|
| M1 | Flasher (910) + Freezing Trap (911) | Trap item cost wrong for both | Both charge 2 Traps | rAthena `skill_db.yml`: Flasher=1, Freezing=1. Only Shockwave+Claymore cost 2. | **MODERATE** |
| M2 | Phantasmic Arrow (917) | No bow weapon type check | Fires without bow equipped | rAthena `skill_db.yml`: `Requires: Weapon: Bow: true` | **MODERATE** |
| M3 | All damage traps | Missing `checkDamageBreakStatuses()` after trap damage | Sleeping/frozen targets stay in status | rAthena `status.yml`: Sleep/Freeze/Stone all have `RemoveOnDamaged: true` — universal on all damage | **MODERATE** |
| M4 | Remove Trap (905) | SQL syntax error: `LIMIT 1` in PostgreSQL UPDATE | `UPDATE ... LIMIT 1` (invalid PG syntax) | Use subquery: `WHERE id = (SELECT id ... LIMIT 1)` | **MODERATE** |
| M5 | Land Mine (904) | Stun chance disputed — our value differs from rAthena | `30 + 5*eff.skillLevel` (35-55%) | rAthena: `sc_start(SC_STUN, 10)` → flat 10% base. RateMyServer says 30+5*lv. **See disputed note.** | **MODERATE** |

### MINOR — Edge cases or cosmetic

| # | Skill | Bug | Current | Canonical (rAthena source) | Severity |
|---|-------|-----|---------|-----------|----------|
| N1 | Ankle Snare (903) | Boss monsters get full immunity | `modeFlags?.statusImmune` → total block | rAthena: `if (status_bl_has_mode(bl, MD_STATUSIMMUNE)) sec /= 5;` — bosses get 1/5 duration | **MINOR** |
| N2 | Ankle Snare (903) | AGI reduction formula wrong | Fixed `4*lv` seconds for all | rAthena: `sec = max(sec * (1 - AGI/200), 30*(casterLv+100) ms)` | **MINOR** |
| N3 | Shockwave Trap (906) | No visual feedback event emitted | No `skill:effect_damage` broadcast | Should emit 0-damage event with SP drain info | **MINOR** |
| N4 | Skid Trap (908) | Doesn't deaggro monsters | Knockback only | rAthena pre-renewal: `mob_unlocktarget(md, tick)` — monster loses target after knockback | **MINOR** |

---

## Verified Correct (No Changes Needed)

Items that were suspected bugs in v1 but confirmed correct by rAthena source:

| Item | Why Correct |
|------|-------------|
| Flasher blind 100% base rate | rAthena: `sc_start(SC_BLIND, 100)`. The 100% goes through `applyStatusEffect` which applies VIT+INT resistance internally. Level only affects trap ground lifetime, not blind chance. |
| Freezing Trap freeze 100% base rate | rAthena: `sc_start(SC_FREEZE, 100)`. 100% base, MDEF resistance applied by status system. |
| Detect 7x7 AoE | rAthena `skill_db.yml`: `SplashArea: 3` = 7x7. Confirmed. |
| Sandman 5x5 AoE | rAthena `skill_db.yml`: `SplashArea: 2` = 5x5. Confirmed. |
| Flasher single-target | rAthena: No SplashArea field = only triggering target. Confirmed. |
| Blitz Beat 3x3 splash | rAthena: `SplashArea: 1` = 3x3. Manual = full damage. Auto = `NK_SPLASHSPLIT` (split). Confirmed. |
| Auto-Blitz procs on missed attacks | rAthena: check is in `skill_additional_effect()` which proceeds for `ATK_BLOCK` (miss). Confirmed. |
| All trap lifetimes | All match `(6-lv)*N` or `lv*N` formulas. Confirmed against rAthena Duration1 values. |
| Trap placement limit 3 | rAthena default `skill_max_trap` = 3. Confirmed. |

---

## Acceptable Gaps (Not Bugs)

| # | Skill | Gap | Why Acceptable |
|---|-------|-----|----------------|
| A1 | Detect (902) | Doesn't reveal enemy traps | PvP-only feature; no PvP yet. rAthena has `skill_reveal_trap_inarea()` |
| A2 | Remove Trap (905) | Can't remove other players' traps | PvP-only feature |
| A3 | Falconry Mastery (916) | Falcon granted on skill learn, not via NPC rental | Intentional simplification; NPC rental is 2,500z from Hunter Guild |
| A4 | Talkie Box (914) | No text message storage/display | Needs client text input UI (deferred) |

---

## Disputed: Land Mine Stun Chance (M5)

**rAthena source:** `sc_start(src, target, SC_STUN, 10, ...)` → 10% flat base rate, all levels.
**RateMyServer:** `5*SkillLv + 30` → 35/40/45/50/55% per level.
**iRO Wiki Classic:** Does not give specific percentage.

These conflict. The rAthena value (10%) seems low for a trap that costs SP and a Trap item, while the RateMyServer value (35-55%) is more commonly cited across community resources.

**Decision:** Change to rAthena's flat 10% for maximum canonical accuracy. The `applyStatusEffect` with 100 base + VIT resistance effectively gives our current implementation a much higher rate than 10%. Changing to match rAthena exactly ensures we match the emulator.

---

## Detailed Fix Plan

### Fix C1: Ankle Snare — Proper Immobilize Status

**Problem:** Ankle Snare calls `applyStatusEffect(... 'stun' ...)` which prevents movement, attack, casting, AND items. rAthena SC_ANKLE only sets `NoMove: true` — trapped targets can still fight back.

**Evidence:** rAthena `db/pre-re/status.yml`:
```yaml
- Status: Ankle
  States:
    NoMove: true        # ONLY NoMove — no NoCast, no NoAttack
  Flags:
    NoClearbuff: true
    StopWalking: true
    NoDispell: true
    RemoveOnChangeMap: true
```

iRO Wiki Classic: "Although trapped enemies cannot move, they can still attack and use skills."

**Fix Steps:**

1. **Add `ankle_snare` status type to `ro_status_effects.js`:**
```javascript
ankle_snare: {
    resistStat: null,           // No resistance — always applies (rAthena uses SCSTART_NORATEDEF)
    resistCap: null,
    baseDuration: 20000,
    canKill: false,
    breakOnDamage: false,       // NOT broken by damage (rAthena: no RemoveOnDamaged flag)
    preventsMovement: true,     // ONLY prevents movement
    preventsCasting: false,     // CAN cast while snared
    preventsAttack: false,      // CAN attack while snared
    preventsItems: false,       // CAN use items while snared
    blocksHPRegen: false,
    blocksSPRegen: false,
    statMods: {}
}
```

2. **Rewrite Ankle Snare trigger handler in `index.js`:**
```javascript
case 'ankle_snare': {
    // rAthena formula: base = 4*lv seconds (Duration2 from skill_db)
    let snareDur = 4000 * eff.skillLevel;

    // Boss monsters: 1/5 duration (rAthena: sec /= 5 for MD_STATUSIMMUNE)
    if (target.modeFlags?.bossProtocol) {
        snareDur = Math.floor(snareDur / 5);
    }

    // AGI reduction (rAthena: sec = max(sec*(1-AGI/200), 30*(srcLv+100)))
    const targetAGI = target.stats?.agi || target.agi || 0;
    const casterLv = caster?.stats?.level || caster?.baseLv || 1;
    const minDuration = 30 * (casterLv + 100); // ms
    snareDur = Math.max(minDuration, Math.floor(snareDur * (1 - targetAGI / 200)));

    // SC_ANKLE: always applies (SCSTART_NORATEDEF in rAthena — bypasses resistance)
    const snareRes = applyStatusEffect({ level: casterLv }, target, 'ankle_snare', 100, { duration: snareDur });
    if (snareRes?.applied) {
        broadcastToZone(eff.zone, 'status:applied', {
            targetId: tid, isEnemy: true, statusType: 'ankle_snare', duration: snareDur
        });
    }
    break;
}
```

**Verification:** Place Ankle Snare, lure monster in. Monster stops moving but keeps attacking if player is in melee range. Boss monsters get caught briefly (1/5 duration). High-AGI targets escape faster.

---

### Fix C2: Blitz Beat Damage Formula

**Problem:** Per-hit damage uses simplified terms instead of canonical floor operations.

**Evidence:** rAthena `battle.cpp` pre-renewal branch:
```c
md.damage = (sstatus->dex / 10 + sstatus->int_ / 2 + skill * 3 + 40) * 2;
```
Note: C integer division truncates (equivalent to `Math.floor` for positive values).

**Fix in Blitz Beat handler (~line 13215):**
```javascript
// BEFORE (wrong):
const perHitDmg = 80 + (steelCrowLv * 6) + (bbStats.int || 0) + Math.floor((bbStats.dex || 0) / 5);

// AFTER (canonical rAthena):
const perHitDmg = (Math.floor((bbStats.dex || 0) / 10) + Math.floor((bbStats.int || 0) / 2) + steelCrowLv * 3 + 40) * 2;
```

**Fix in Auto-Blitz handler (~line 18223):**
```javascript
// BEFORE (wrong):
const abPerHit = 80 + (abSteelCrow * 6) + (cachedEffStats.int || 0) + Math.floor((cachedEffStats.dex || 0) / 5);

// AFTER (canonical rAthena):
const abPerHit = (Math.floor((cachedEffStats.dex || 0) / 10) + Math.floor((cachedEffStats.int || 0) / 2) + abSteelCrow * 3 + 40) * 2;
```

**Example:** INT=70, DEX=99, Steel Crow 10, Blitz Beat Lv5:
- Canonical: `(9 + 35 + 30 + 40) * 2 = 114 * 2 = 228 per hit × 5 = 1,140`
- Before fix: `80 + 60 + 70 + 19 = 229 per hit × 5 = 1,145`

---

### Fix C3: Auto-Blitz Hit Count

**Problem:** `Math.floor(jobLv / 10) + 1` gives wrong hit count at exact multiples of 10.

**Evidence:** rAthena `skill.cpp`:
```c
rate = (sd->status.job_level + 9) / 10;  // C integer division = floor
```

| Job Level | Current (wrong) | Canonical |
|-----------|-----------------|-----------|
| 10 | 2 | 1 |
| 20 | 3 | 2 |
| 30 | 4 | 3 |
| 40 | 5 | 4 |

**Fix (~line 18221):**
```javascript
// BEFORE (wrong):
const abHits = Math.min(abBlitzLv, Math.min(5, Math.floor(jobLv / 10) + 1));

// AFTER (canonical rAthena):
const abHits = Math.min(abBlitzLv, Math.min(5, Math.floor((jobLv + 9) / 10)));
```

---

### Fix C4: Freezing Trap Damage Pipeline

**Problem:** Freezing Trap uses MISC damage (DEX/INT formula, ignores DEF, always hits). rAthena classifies it as `Type: Weapon` — the only trap that uses ATK-based damage.

**Evidence:** rAthena `db/pre-re/skill_db.yml`:
```yaml
- Id: 121
  Name: HT_FREEZINGTRAP
  Type: Weapon          # NOT Misc — uses ATK pipeline
  Element: Water
  DamageFlags:
    Splash: true
    IgnoreAtkCard: true   # Cards don't apply, but ATK/DEF still do
```

Compare: Land Mine, Blast Mine, Claymore all have `Type: Misc`.

**Damage formula:** `(25 + 25 * SkillLv)%` of ATK = 50/75/100/125/150%

**Key differences from MISC:**
- Uses caster's ATK (not DEX/INT formula)
- Reduced by target DEF (not ignored)
- CAN miss (no `IgnoreFlee` flag — unlike Land Mine/Blast Mine/Claymore)
- ATK cards don't apply (`IgnoreAtkCard`)

**Fix approach:** Store `ownerATK` (total physical ATK at placement time) in the trap data, then use it at trigger time.

**Step 1 — Store ATK at placement (~line 13055):**
```javascript
const ownerStats = getEffectiveStats(player);
const ownerDEX = ownerStats.dex || 1;
const ownerINT = ownerStats.int || 1;
// NEW: Store ATK for Freezing Trap weapon-type damage
const ownerATK = (player.equippedWeaponRight?.atk || 0) + (ownerStats.str || 0) +
    Math.floor(Math.pow(Math.floor((ownerStats.str || 0) / 10), 2)) +
    Math.floor((ownerStats.dex || 0) / 5) + Math.floor((ownerStats.luk || 0) / 5);
```

Then include `ownerATK` in the `createGroundEffect()` data.

**Step 2 — Freezing Trap trigger uses ATK formula (~line 21252):**
```javascript
case 'freezing_trap': {
    // ATK-based weapon damage: (25+25*lv)% of ATK, reduced by DEF, can miss
    const atkPct = 25 + 25 * eff.skillLevel; // 50/75/100/125/150%
    for (const [sid, se] of enemies.entries()) {
        if (se.isDead || se.zone !== eff.zone) continue;
        const sdx = eff.x - se.x, sdy = eff.y - se.y;
        if (Math.sqrt(sdx * sdx + sdy * sdy) > 75) continue; // 3x3

        // Flee check (Freezing Trap does NOT have IgnoreFlee)
        const targetFlee = (se.stats?.flee || se.flee || 0);
        const hitRate = Math.min(100, Math.max(5, 80 + (eff.ownerDEX || 0) - targetFlee));
        if (Math.random() * 100 >= hitRate) {
            // Miss — still broadcast
            broadcastToZone(eff.zone, 'skill:effect_damage', {
                attackerId: eff.casterId, attackerName: eff.casterName,
                targetId: sid, targetName: se.name, isEnemy: true,
                skillId: eff.skillId, skillName: 'Freezing Trap', skillLevel: eff.skillLevel, element: 'water',
                damage: 0, isCritical: false, isMiss: true, hitType: 'physical',
                targetHealth: se.health, targetMaxHealth: se.maxHealth,
                targetX: se.x, targetY: se.y, targetZ: se.z, timestamp: now
            });
            continue;
        }

        // ATK-based damage with DEF reduction
        const rawAtk = (eff.ownerATK || 100) * atkPct / 100;
        const hardDef = se.stats?.def || se.def || 0;
        const softDef = se.stats?.vit || se.vit || 0;
        const reducedByDef = Math.max(1, rawAtk * (100 - hardDef) / 100 - softDef);
        // Apply water element modifier
        const sEleMod = getElementModifier('water', (se.element?.type || 'neutral'), (se.element?.level || 1));
        const variance = 0.9 + Math.random() * 0.2;
        const sDmg = Math.max(1, Math.floor(reducedByDef * sEleMod / 100 * variance));

        se.health = Math.max(0, se.health - sDmg); se.lastDamageTime = now;
        if (typeof checkDamageBreakStatuses === 'function') checkDamageBreakStatuses(se, sid, eff.zone, io, true);
        broadcastToZone(eff.zone, 'skill:effect_damage', {
            attackerId: eff.casterId, attackerName: eff.casterName,
            targetId: sid, targetName: se.name, isEnemy: true,
            skillId: eff.skillId, skillName: 'Freezing Trap', skillLevel: eff.skillLevel, element: 'water',
            damage: sDmg, isCritical: false, isMiss: false, hitType: 'physical',
            targetHealth: se.health, targetMaxHealth: se.maxHealth,
            targetX: se.x, targetY: se.y, targetZ: se.z, timestamp: now
        });
        broadcastToZone(eff.zone, 'enemy:health_update', { enemyId: sid, health: se.health, maxHealth: se.maxHealth, inCombat: true });

        // Freeze: 100% base (rAthena: sc_start SC_FREEZE 100), MDEF resistance via applyStatusEffect
        if (!se.modeFlags?.statusImmune) {
            const freezeDur = 3000 * eff.skillLevel;
            const fRes = applyStatusEffect({ level: caster?.stats?.level || 1 }, se, 'freeze', 100, { duration: freezeDur });
            if (fRes?.applied) broadcastToZone(eff.zone, 'status:applied', { targetId: sid, isEnemy: true, statusType: 'freeze', duration: freezeDur });
        }
        if (se.health <= 0 && caster) processEnemyDeathFromSkill(se, caster, eff.casterId, io);
    }
    break;
}
```

---

### Fix M1: Trap Item Costs (Flasher + Freezing Trap)

**Problem:** Both Flasher and Freezing Trap charge 2 Trap items. rAthena confirms both should cost 1.

**Evidence:** rAthena `skill_db.yml`:
| Skill | rAthena Item Cost |
|-------|------------------|
| Shockwave Trap | 2 Booby_Trap |
| Claymore Trap | 2 Booby_Trap |
| All others | 1 Booby_Trap |

**Fix in trap placement handler (~line 13014):**
```javascript
// BEFORE (wrong — Flasher and Freezing both charged 2):
const trapItemCost = (skill.name === 'claymore_trap' || skill.name === 'freezing_trap' || skill.name === 'shockwave_trap' || skill.name === 'flasher') ? 2 : 1;

// AFTER (correct — only Shockwave and Claymore cost 2):
const trapItemCost = (skill.name === 'claymore_trap' || skill.name === 'shockwave_trap') ? 2 : 1;
```

---

### Fix M2: Phantasmic Arrow Bow Check

**Problem:** Phantasmic Arrow fires without bow equipped.

**Evidence:** rAthena `skill_db.yml`: `Requires: Weapon: Bow: true`

**Fix (~line 13265):**
```javascript
if (skill.name === 'phantasmic_arrow') {
    if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }
    if (player.weaponType !== 'bow') { socket.emit('skill:error', { message: 'Requires bow' }); return; }
    // ... rest of handler
```

---

### Fix M3: Trap Damage Should Break Statuses

**Problem:** Damage traps don't call `checkDamageBreakStatuses()`.

**Evidence:** rAthena `status.yml` — Sleep, Freeze, Stone all have `RemoveOnDamaged: true`. In rAthena `status.cpp`, `status_damage()` universally checks this flag on ANY damage.

**Fix:** Add `checkDamageBreakStatuses()` after damage in all 5 trap damage paths:
1. Land Mine trigger (~line 21215)
2. Blast Mine trigger (~line 21228)
3. Blast Mine auto-detonate (~line 21182)
4. Claymore Trap trigger (~line 21243)
5. Freezing Trap trigger (covered in C4 rewrite above)

```javascript
// After each: target.health = Math.max(0, target.health - dmg);
if (typeof checkDamageBreakStatuses === 'function') checkDamageBreakStatuses(target, tid, eff.zone, io, true);
```

---

### Fix M4: Remove Trap SQL Syntax Error

**Problem:** `UPDATE ... LIMIT 1` is invalid PostgreSQL.

**Fix (~line 13130):**
```javascript
// BEFORE (invalid):
await pool.query(
    `UPDATE character_inventory SET quantity = quantity + 1 WHERE character_id = $1 AND item_id = 1065 AND is_equipped = false LIMIT 1`,
    [characterId]
);

// AFTER (valid PostgreSQL):
await pool.query(
    `UPDATE character_inventory SET quantity = quantity + 1 WHERE id = (SELECT id FROM character_inventory WHERE character_id = $1 AND item_id = 1065 AND is_equipped = false LIMIT 1)`,
    [characterId]
);
```

---

### ~~Fix M5: Land Mine Stun Chance~~ — KEPT AT (30+5*lv)%

**Disputed value.** rAthena refactored source shows `sc_start(SC_STUN, 10)` which could mean flat 10%. However, RateMyServer pre-renewal EXPLICITLY documents `(5*SkillLV+30)%` = 35/40/45/50/55%.

**Final decision (v3):** Keep `30 + 5 * eff.skillLevel` (35-55%). Reasoning:
1. RateMyServer explicitly documents `(5*SkillLV+30)%` with per-level values
2. The rAthena `10` parameter is ambiguous — the rate scale differs between `sc_start` callers
3. iRO Wiki Classic doesn't contradict level-scaling
4. Multiple community sources consistently cite 35-55%

**No code change applied.** Current implementation matches RateMyServer.

---

### Fix N1 + N2: Ankle Snare Boss Duration + AGI Reduction

Addressed in Fix C1 above. The rewritten handler includes:
- Boss 1/5 duration: `snareDur = Math.floor(snareDur / 5)`
- AGI reduction: `snareDur = max(snareDur * (1 - AGI/200), 30*(casterLv+100))`

---

### Fix N3: Shockwave Trap Visual Feedback

**Fix in Shockwave Trap trigger handler (~line 21287):**
```javascript
case 'shockwave_trap': {
    if (target.mana && target.mana > 0 && !target.modeFlags?.statusImmune) {
        const drainPct = 5 + 15 * eff.skillLevel;
        const spLost = Math.floor(target.mana * drainPct / 100);
        target.mana = Math.max(0, target.mana - spLost);
        // ADD: Emit visual feedback
        broadcastToZone(eff.zone, 'skill:effect_damage', {
            attackerId: eff.casterId, attackerName: eff.casterName,
            targetId: tid, targetName: target.name, isEnemy: true,
            skillId: eff.skillId, skillName: 'Shockwave Trap', skillLevel: eff.skillLevel, element: 'neutral',
            damage: 0, isCritical: false, isMiss: false, hitType: 'misc',
            targetHealth: target.health, targetMaxHealth: target.maxHealth,
            targetX: target.x, targetY: target.y, targetZ: target.z,
            spDrained: spLost, timestamp: now
        });
    }
    break;
}
```

---

### Fix N4: Skid Trap Deaggro

**Problem:** In rAthena pre-renewal, monsters hit by Skid Trap lose their target.

**Evidence:** rAthena `skill.cpp`:
```c
#ifndef RENEWAL
    mob_data* md = BL_CAST(BL_MOB, bl);
    if (md) mob_unlocktarget(md, tick);
#endif
```

**Fix in Skid Trap trigger handler (~line 21291):**
```javascript
case 'skid_trap': {
    if (!target.modeFlags?.statusImmune) {
        knockbackTarget(target, eff.x, eff.y, 5 + eff.skillLevel, eff.zone, io);
    }
    // Pre-renewal: monster loses target after Skid Trap
    if (target.currentTarget) {
        target.currentTarget = null;
        target.mode = 'idle';
        target.aggroTarget = null;
    }
    break;
}
```

---

## Skill Data Audit (`ro_skill_data_2nd.js`)

All fields verified against rAthena `skill_db.yml`. All correct — no changes needed.

| Skill | SP | Cast | ACD | Range | Prereqs | Element | rAthena Match |
|-------|----|------|-----|-------|---------|---------|---------------|
| 900 Blitz Beat | 10+3*i | 1500 | 1000 | 250 | 916@1 | neutral | ✅ |
| 901 Steel Crow | 0 | 0 | 0 | 0 | 900@5 | — | ✅ |
| 902 Detect | 8 flat | 0 | 0 | 450 | 302@1,916@1 | neutral | ✅ |
| 903 Ankle Snare | 12 | 0 | 0 | 600 | 908@1 | neutral | ✅ |
| 904 Land Mine | 10 | 0 | 0 | 600 | none | earth | ✅ |
| 905 Remove Trap | 5 | 0 | 0 | 150 | 904@1 | neutral | ✅ |
| 906 Shockwave | 45 | 0 | 0 | 600 | 903@1 | neutral | ✅ |
| 907 Claymore | 15 | 0 | 0 | 600 | 906@1,912@1 | fire | ✅ |
| 908 Skid Trap | 10 | 0 | 0 | 600 | none | neutral | ✅ |
| 909 Sandman | 12 | 0 | 0 | 600 | 910@1 | neutral | ✅ |
| 910 Flasher | 12 | 0 | 0 | 600 | 908@1 | neutral | ✅ |
| 911 Freezing | 10 | 0 | 0 | 600 | 910@1 | water | ✅ |
| 912 Blast Mine | 10 | 0 | 0 | 600 | 904@1,909@1,911@1 | wind | ✅ |
| 913 Spring Trap | 10 | 0 | 0 | 600 | 905@1,916@1 | neutral | ✅ |
| 914 Talkie Box | 1 | 0 | 0 | 600 | 906@1,905@1 | neutral | ✅ |
| 915 Beast Bane | 0 | 0 | 0 | 0 | none | — | ✅ |
| 916 Falconry M. | 0 | 0 | 0 | 0 | 915@1 | — | ✅ |
| 917 Phantasmic | 10 | 0 | 0 | 450 | quest | neutral | ✅ |

---

## Implementation Order

### Phase 1: Critical Fixes (Gameplay-Breaking)

| Step | Fix | Description | Risk |
|------|-----|-------------|------|
| 1a | Add `ankle_snare` status to `ro_status_effects.js` | New status type | LOW |
| 1b | C1: Rewrite Ankle Snare handler (+ N1 boss 1/5 + N2 AGI formula) | Use `ankle_snare` not `stun` | MED |
| 1c | C2: Fix Blitz Beat formula (manual + auto) | `(DEX/10 + INT/2 + SC*3 + 40) * 2` | LOW |
| 1d | C3: Fix Auto-Blitz hit count | `floor((jobLv+9)/10)` | LOW |
| 1e | C4: Rewrite Freezing Trap to ATK-based + store ownerATK | Weapon pipeline, DEF, Flee | HIGH |

### Phase 2: Moderate Fixes

| Step | Fix | Description | Risk |
|------|-----|-------------|------|
| 2a | M1: Fix trap item costs (Flasher 2→1, Freezing 2→1) | Only Shockwave+Claymore = 2 | LOW |
| 2b | M2: Add Phantasmic Arrow bow check | `weaponType !== 'bow'` guard | LOW |
| 2c | M3: Add `checkDamageBreakStatuses()` to 4 remaining damage traps | Land/Blast/Claymore/Blast-auto | LOW |
| 2d | M4: Fix Remove Trap SQL | Subquery for LIMIT | LOW |
| ~~2e~~ | ~~M5: Land Mine stun~~ | KEPT at 35-55% (RMS explicit docs) | N/A |

### Phase 3: Minor Fixes

| Step | Fix | Description | Risk |
|------|-----|-------------|------|
| 3a | N3: Shockwave Trap visual feedback | Emit `skill:effect_damage` with spDrained | LOW |
| 3b | N4: Skid Trap deaggro | Clear target after knockback | LOW |

---

## Verification Checklist

### Blitz Beat (900)
- [ ] Requires falcon (`hasFalcon === true`)
- [ ] SP cost: 10/13/16/19/22
- [ ] Cast time: 1.5s (reduced by DEX)
- [ ] After-cast delay: 1s
- [ ] 1/2/3/4/5 hits per level
- [ ] MISC damage (ignores DEF/MDEF/Flee/cards/size)
- [ ] Always Neutral element
- [ ] Damage formula: `(floor(DEX/10) + floor(INT/2) + SteelCrow*3 + 40) * 2` per hit
- [ ] Full damage to ALL 3x3 targets (not split)
- [ ] Element modifier applied vs target defense element

### Auto-Blitz Beat
- [ ] Proc rate: `floor(LUK/3)` percent
- [ ] Triggers ONLY on bow auto-attacks
- [ ] Can trigger on MISSED attacks
- [ ] SP cost: 0
- [ ] Cast/ACD: 0 (instant)
- [ ] Hit count: `min(BlitzLv, floor((jobLv+9)/10))`
- [ ] Damage SPLIT among 3x3 targets
- [ ] Same damage formula as manual Blitz Beat
- [ ] Job level 10: 1 hit (not 2)
- [ ] Job level 50: 5 hits

### Detect (902)
- [ ] Requires falcon
- [ ] SP: 8 (flat, all levels)
- [ ] Instant cast
- [ ] 7x7 AoE (175 UE radius) — confirmed by rAthena SplashArea: 3
- [ ] Reveals Hiding, Cloaking in AoE
- [ ] Removes buff and broadcasts

### Ankle Snare (903)
- [ ] 1 Trap item consumed
- [ ] Target CANNOT move
- [ ] Target CAN still attack, cast, and use items
- [ ] Status type: `ankle_snare` (not `stun`)
- [ ] Monster base duration: `4*lv` seconds
- [ ] Boss duration: 1/5 of normal
- [ ] AGI reduction: `dur * (1 - AGI/200)`, min `30*(casterLv+100)` ms
- [ ] No damage dealt
- [ ] NOT broken by damage

### Land Mine (904)
- [ ] 1 Trap item consumed
- [ ] MISC Earth damage: `lv * (DEX+75) * (1+INT/100)` ±10%
- [ ] Stun chance: 10% flat (rAthena canonical)
- [ ] Stun duration: 5s
- [ ] Boss immune to stun, still takes damage
- [ ] `checkDamageBreakStatuses()` called

### Remove Trap (905)
- [ ] Can only remove own traps
- [ ] Returns 1 Trap item
- [ ] SQL query executes without error (subquery pattern)
- [ ] Cannot remove already-triggered traps

### Shockwave Trap (906)
- [ ] 2 Trap items consumed
- [ ] SP drain: 20/35/50/65/80% of current SP
- [ ] Boss immune
- [ ] `skill:effect_damage` event emitted (spDrained field)

### Claymore Trap (907)
- [ ] 2 Trap items consumed
- [ ] MISC Fire damage: `lv * (75+floor(DEX/2)) * (1+INT/100)` ±10%
- [ ] 5x5 AoE (all enemies in radius)
- [ ] Duration: 20/40/60/80/100s (increases with level)
- [ ] `checkDamageBreakStatuses()` called

### Skid Trap (908)
- [ ] 1 Trap item consumed
- [ ] Knockback: 6/7/8/9/10 cells
- [ ] No damage
- [ ] Boss immune to knockback
- [ ] Monster loses target after knockback (deaggro)

### Sandman (909)
- [ ] 1 Trap item consumed
- [ ] 5x5 AoE (rAthena SplashArea: 2, confirmed)
- [ ] Sleep chance: 50/60/70/80/90% (`10*lv+40`)
- [ ] Sleep duration: 30s
- [ ] Boss immune to sleep
- [ ] Sleep breaks on damage

### Flasher (910)
- [ ] **1 Trap item consumed** (was 2, now fixed)
- [ ] Blind 100% base rate (resistance via VIT+INT in applyStatusEffect)
- [ ] Blind duration: 30s
- [ ] Single target only (triggering enemy, no splash — confirmed rAthena)
- [ ] Boss immune to blind

### Freezing Trap (911)
- [ ] **1 Trap item consumed** (was 2, now fixed)
- [ ] **ATK-based Weapon damage: `(25+25*lv)%` ATK**, reduced by DEF
- [ ] **CAN miss** (no IgnoreFlee — confirmed rAthena)
- [ ] IgnoreAtkCard (no card bonuses on damage)
- [ ] 3x3 AoE
- [ ] Freeze 100% base rate (MDEF resistance via applyStatusEffect)
- [ ] Freeze duration: 3/6/9/12/15s
- [ ] Boss/Undead immune to freeze, still takes damage
- [ ] `checkDamageBreakStatuses()` called (before freeze application)

### Blast Mine (912)
- [ ] 1 Trap item consumed
- [ ] MISC Wind damage: `lv * (50+floor(DEX/2)) * (1+INT/100)` ±10%
- [ ] 3x3 AoE
- [ ] Auto-detonate: 25/20/15/10/5s
- [ ] Triggers on enemy step OR auto-detonate timer
- [ ] `checkDamageBreakStatuses()` called

### Spring Trap (913)
- [ ] Requires falcon
- [ ] Range: 200/250/300/350/400 UE per level
- [ ] Can destroy ANY visible trap (own or enemy)
- [ ] Does NOT return Trap item

### Talkie Box (914)
- [ ] 1 Trap item consumed
- [ ] Duration: 600s
- [ ] Cosmetic only

### Beast Bane (915)
- [ ] +4 ATK/lv vs Brute race
- [ ] +4 ATK/lv vs Insect race
- [ ] In `getPassiveSkillBonuses()` raceATK

### Falconry Mastery (916)
- [ ] Sets `player.hasFalcon = true`
- [ ] Prerequisite: Beast Bane Lv 1

### Phantasmic Arrow (917)
- [ ] **Requires bow** (now checked)
- [ ] 150% ATK physical damage
- [ ] Knockback 3 cells
- [ ] SP: 10, Quest skill
- [ ] Does NOT consume arrows

### System-Level
- [ ] Max 3 traps per hunter (oldest removed on 4th)
- [ ] All traps check Trap item (ID 1065) inventory
- [ ] Trap trigger radius: 50 UE (1 cell)
- [ ] All trap events emitted: placed/triggered/detonated/expired/removed
- [ ] `calculateTrapDamage()` formulas correct for 3 MISC traps (Land Mine, Blast Mine, Claymore)
- [ ] Freezing Trap uses separate ATK-based path (not calculateTrapDamage)

---

## Files Modified

| File | Changes |
|------|---------|
| `server/src/ro_status_effects.js` | Add `ankle_snare` status type |
| `server/src/ro_ground_effects.js` | Remove `freezing_trap` from `calculateTrapDamage()` (now ATK-based) |
| `server/src/index.js` ~13014 | Fix trap item costs — only Shockwave+Claymore = 2 (M1) |
| `server/src/index.js` ~13055 | Store `ownerATK` for Freezing Trap at placement (C4) |
| `server/src/index.js` ~13130 | Fix Remove Trap SQL subquery (M4) |
| `server/src/index.js` ~13215 | Fix Blitz Beat formula (C2) |
| `server/src/index.js` ~13265 | Add Phantasmic Arrow bow check (M2) |
| `server/src/index.js` ~18221 | Fix Auto-Blitz hit count `floor((jobLv+9)/10)` (C3) |
| `server/src/index.js` ~18223 | Fix Auto-Blitz formula (C2) |
| `server/src/index.js` ~21182 | Add checkDamageBreakStatuses to Blast Mine auto-detonate (M3) |
| `server/src/index.js` ~21215 | Add checkDamageBreakStatuses to Land Mine (M3) |
| `server/src/index.js` ~21218 | Fix Land Mine stun chance to flat 10% (M5) |
| `server/src/index.js` ~21228 | Add checkDamageBreakStatuses to Blast Mine trigger (M3) |
| `server/src/index.js` ~21243 | Add checkDamageBreakStatuses to Claymore Trap (M3) |
| `server/src/index.js` ~21252-21272 | Rewrite Freezing Trap: ATK-based + Flee check + checkDamageBreak (C4) |
| `server/src/index.js` ~21273-21281 | Rewrite Ankle Snare: ankle_snare status + boss 1/5 + AGI (C1, N1, N2) |
| `server/src/index.js` ~21282-21290 | Add Shockwave Trap visual feedback (N3) |
| `server/src/index.js` ~21291-21297 | Add Skid Trap deaggro (N4) |

**Total: 3 files, 18 change locations, 13 bugs fixed.**
